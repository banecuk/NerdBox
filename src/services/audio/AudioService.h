#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "services/audio/AudioData.h"
#include "utils/logging/LoggerInterface.h"

// Parses push POST bodies from the mb_NerdBox MusicBee plugin (see
// docs-local/NERDBOX_INTEGRATION.md) and applies them to the shared
// AudioData struct. Owned by ServiceBundle; called from
// WebServerService's POST /audio handler on the main-loop task.
class AudioService {
 public:
    AudioService(AudioData& data, LoggerInterface& logger);

    // Parses one push event body (len bytes, need not be NUL-terminated) and
    // applies it to the AudioData passed at construction. nowMs is the
    // caller's millis(), injected so the protocol logic is host-testable
    // (test/AudioServiceTest.cpp). Returns true if the caller should request
    // a resync (HTTP 409 / a `resend` response body, per the plugin's
    // protocol): a seq gap or trackId mismatch was detected, or the body
    // failed to parse. An unknown `event` value is ignored entirely (no
    // bookkeeping or freshness update) and doesn't request a resync.
    bool handlePush(const char* body, size_t len, uint32_t nowMs);

 private:
    static AudioData::PlayState parsePlayState(const char* name);

    AudioData& data_;
    LoggerInterface& logger_;

    // Heap-allocated per project convention (avoid a large JsonDocument on
    // the stack); reused across pushes to avoid heap fragmentation. Payloads
    // are small (~300 B) so no filter is needed.
    std::unique_ptr<JsonDocument> doc_;

    // Explicit "has a push event been processed yet" flag, rather than
    // inferring it from data_.seq/trackId being non-zero — self-documenting,
    // and correct even in the practically-impossible case of seq wrapping
    // back to exactly 0.
    bool seenAnyEvent_ = false;

    // trackId of the last `track` event (the only event carrying metadata).
    // Heartbeats are checked against this — see handlePush().
    uint32_t trackEventTrackId_ = 0;
};
