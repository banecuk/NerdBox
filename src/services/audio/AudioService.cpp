#include "AudioService.h"

#include <Arduino.h>
#include <cstring>

#include "utils/logging/LogMacros.h"

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

bool AudioService::handlePush(const String& body) {
    doc_->clear();
    const DeserializationError err = deserializeJson(*doc_, body);
    if (err) {
        logger_.errorf("[Audio] Failed to parse push body: %s", err.c_str());
        // Malformed delivery — ask the plugin to resend a full track event
        // on its next tick rather than silently drifting out of sync.
        return true;
    }

    const char* event = (*doc_)["event"] | "";
    const uint32_t seq = (*doc_)["seq"] | 0;
    const uint32_t trackId = (*doc_)["trackId"] | 0;
    const uint32_t session = (*doc_)["session"] | 0;

    bool needsResync = false;
    if (seenAnyEvent_ && data_.session == session) {
        if (seq != data_.seq + 1) {
            LOG_DEBUGF(logger_, "[Audio] seq gap: expected %lu, got %lu",
                      static_cast<unsigned long>(data_.seq + 1), static_cast<unsigned long>(seq));
            needsResync = true;
        }
        const bool isHeartbeat = strcmp(event, "ping") == 0 || strcmp(event, "paused") == 0;
        if (isHeartbeat && data_.hasTrack && trackId != data_.trackId) {
            LOG_DEBUG(logger_, "[Audio] trackId mismatch on heartbeat - missed a track event");
            needsResync = true;
        }
    }
    // A session change (including the very first event ever received) is a
    // fresh plugin run, not a miss — don't flag it, per
    // docs-local/NERDBOX_INTEGRATION.md's "Detecting missed events".

    data_.seq = seq;
    data_.trackId = trackId;
    data_.session = session;
    seenAnyEvent_ = true;

    const uint32_t now = millis();

    if (strcmp(event, "track") == 0) {
        const char* title = (*doc_)["title"] | "";
        const char* artist = (*doc_)["artist"] | "";
        const char* album = (*doc_)["album"] | "";
        const char* format = (*doc_)["format"] | "";
        const char* bitrate = (*doc_)["bitrate"] | "";
        snprintf(data_.title, sizeof(data_.title), "%s", title);
        snprintf(data_.artist, sizeof(data_.artist), "%s", artist);
        snprintf(data_.album, sizeof(data_.album), "%s", album);
        snprintf(data_.format, sizeof(data_.format), "%s", format);
        snprintf(data_.bitrate, sizeof(data_.bitrate), "%s", bitrate);
        data_.durationMs = (*doc_)["durationMs"] | 0;
        data_.positionMs = (*doc_)["positionMs"] | 0;
        const char* playState = (*doc_)["playState"] | "";
        data_.playState = parsePlayState(playState);
        data_.isPlaying = (*doc_)["isPlaying"] | false;
        data_.hasTrack = true;
        data_.stopped = false;
        data_.offline = false;
    } else if (strcmp(event, "ping") == 0) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Playing;
        data_.isPlaying = true;
        data_.stopped = false;
    } else if (strcmp(event, "paused") == 0) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Paused;
        data_.isPlaying = false;
        data_.stopped = false;
    } else if (strcmp(event, "stop") == 0) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Stopped;
        data_.isPlaying = false;
        data_.stopped = true;
        data_.offline = false;
        data_.stoppedAtMs = now;
    } else if (strcmp(event, "offline") == 0) {
        data_.positionMs = (*doc_)["positionMs"] | 0;
        data_.playState = AudioData::PlayState::Stopped;
        data_.isPlaying = false;
        data_.stopped = true;
        data_.offline = true;
        data_.stoppedAtMs = now;
    } else {
        // Not necessarily a fault — could be a future protocol addition
        // the device doesn't know about yet — so this is a warning, not
        // an error.
        logger_.warningf("[Audio] Unknown push event: %s", event);
    }

    data_.freshness.publish(now);

    return needsResync;
}
