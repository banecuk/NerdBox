#include "AudioService.h"

#include <cstring>

#include "utils/logging/LogMacros.h"
#include "utils/Utf8.h"

AudioService::AudioService(AudioData& data, LoggerInterface& logger)
    : data_(data), logger_(logger) {
    doc_ = std::make_unique<JsonDocument>();
}

AudioData::PlayState AudioService::parsePlayState(const char* name) {
    if (strcmp(name, "Playing") == 0)
        return AudioData::PlayState::Playing;
    if (strcmp(name, "Paused") == 0)
        return AudioData::PlayState::Paused;
    if (strcmp(name, "Stopped") == 0)
        return AudioData::PlayState::Stopped;
    if (strcmp(name, "Loading") == 0)
        return AudioData::PlayState::Loading;
    return AudioData::PlayState::Undefined;
}

namespace {

enum class Event : uint8_t {
    Unknown,
    Track,
    Ping,
    Paused,
    Stop,
    Offline
};

Event parseEvent(const char* name) {
    if (strcmp(name, "track") == 0)
        return Event::Track;
    if (strcmp(name, "ping") == 0)
        return Event::Ping;
    if (strcmp(name, "paused") == 0)
        return Event::Paused;
    if (strcmp(name, "stop") == 0)
        return Event::Stop;
    if (strcmp(name, "offline") == 0)
        return Event::Offline;
    return Event::Unknown;
}

}  // namespace

bool AudioService::handlePush(const char* body, size_t len, uint32_t nowMs) {
    doc_->clear();
    if (body == nullptr) {
        body = "";
        len = 0;
    }
    const DeserializationError err = deserializeJson(*doc_, body, len);
    if (err) {
        logger_.errorf("[Audio] Failed to parse push body: %s", err.c_str());
        // Malformed delivery — ask the plugin to resend a full track event
        // on its next tick rather than silently drifting out of sync.
        return true;
    }

    const char* eventName = (*doc_)["event"] | "";
    const Event event = parseEvent(eventName);
    if (event == Event::Unknown) {
        // Not necessarily a fault (could be a future protocol addition), so a
        // warning, and no resync. Rejected before the seq/trackId/session
        // bookkeeping: an event we don't understand mustn't advance seq or
        // refresh freshness as if it had been applied.
        logger_.warningf("[Audio] Unknown push event: %s", eventName);
        return false;
    }

    const uint32_t seq = (*doc_)["seq"] | 0;
    const uint32_t trackId = (*doc_)["trackId"] | 0;
    const uint32_t session = (*doc_)["session"] | 0;

    bool needsResync = false;
    if (seenAnyEvent_ && data_.session == session && seq != data_.seq + 1) {
        LOG_DEBUGF(logger_, "[Audio] seq gap: expected %lu, got %lu",
                   static_cast<unsigned long>(data_.seq + 1), static_cast<unsigned long>(seq));
        needsResync = true;
    }
    // A session change (including the very first event ever received) is a
    // fresh plugin run, not a seq gap — don't flag it, per
    // docs-local/NERDBOX_INTEGRATION.md's "Detecting missed events".

    // Heartbeats carry no metadata, so one we can't match to a cached `track`
    // event always needs a resync — whatever the session/seq bookkeeping says:
    //  - no track cached at all: e.g. the device rebooted mid-playback; the
    //    first event it sees is a ping with contiguous seq after that, so
    //    without this the paused/playing track would never be shown (B44);
    //  - trackId differs from the last *track* event's: a missed track
    //    change. Compared against trackEventTrackId_, not data_.trackId,
    //    which every event overwrites — otherwise one lost 409 meant the next
    //    ping matched and stale metadata stuck for the whole track.
    const bool isHeartbeat = event == Event::Ping || event == Event::Paused;
    if (isHeartbeat && (!data_.hasTrack || trackId != trackEventTrackId_)) {
        LOG_DEBUG(logger_, "[Audio] heartbeat without matching track event - requesting resync");
        needsResync = true;
    }

    data_.seq = seq;
    data_.trackId = trackId;
    data_.session = session;
    seenAnyEvent_ = true;

    const uint32_t now = nowMs;

    if (event == Event::Track) {
        trackEventTrackId_ = trackId;
        const char* title = (*doc_)["title"] | "";
        const char* artist = (*doc_)["artist"] | "";
        const char* album = (*doc_)["album"] | "";
        const char* format = (*doc_)["format"] | "";
        const char* bitrate = (*doc_)["bitrate"] | "";
        // UTF-8-aware: a plain snprintf could cut mid code point (B31).
        Utf8::copyTruncated(data_.title, sizeof(data_.title), title);
        Utf8::copyTruncated(data_.artist, sizeof(data_.artist), artist);
        Utf8::copyTruncated(data_.album, sizeof(data_.album), album);
        Utf8::copyTruncated(data_.format, sizeof(data_.format), format);
        Utf8::copyTruncated(data_.bitrate, sizeof(data_.bitrate), bitrate);
        data_.durationMs = (*doc_)["durationMs"] | 0;
        data_.positionMs = (*doc_)["positionMs"] | 0;
        const char* playState = (*doc_)["playState"] | "";
        data_.playState = parsePlayState(playState);
        data_.isPlaying = (*doc_)["isPlaying"] | false;
        data_.hasTrack = true;
        data_.stopped = false;
        data_.offline = false;
    } else if (event == Event::Ping) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Playing;
        data_.isPlaying = true;
        data_.stopped = false;
    } else if (event == Event::Paused) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Paused;
        data_.isPlaying = false;
        data_.stopped = false;
    } else if (event == Event::Stop) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Stopped;
        data_.isPlaying = false;
        data_.stopped = true;
        data_.offline = false;
        data_.stoppedAtMs = now;
    } else {  // Event::Offline
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Stopped;
        data_.isPlaying = false;
        data_.stopped = true;
        data_.offline = true;
        data_.stoppedAtMs = now;
    }

    data_.freshness.publish(now);

    return needsResync;
}
