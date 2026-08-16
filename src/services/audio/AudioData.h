#pragma once

#include <cstdint>

#include "utils/PublishedFlag.h"

// Now-playing state pushed by the mb_NerdBox MusicBee plugin — see
// docs-local/NERDBOX_INTEGRATION.md. Written by AudioService from the web
// server's POST /audio handler (main-loop task), read by AudioWidget/
// MultiWidget (screen task). All fields are scalars/fixed char arrays, no
// heap members, so — like AirQualityData/PcMetrics' scalar fields — no mutex
// is needed; a torn read of a string mid-update is possible but cosmetic and
// self-heals on the next event (every ~1s while a track is loaded).
struct AudioData {
    enum class PlayState : uint8_t { Undefined, Loading, Playing, Paused, Stopped };

    // Stamped on every push event received (track/ping/paused/stop/offline) —
    // not a staleness signal the way PcMetrics/AirQualityData use it, since
    // silence after `stop` is expected/permanent until the next track, not a
    // failure. Kept for /api/status-style debug visibility.
    PublishedFlag freshness;

    char title[96] = {0};
    char artist[64] = {0};
    char album[64] = {0};
    char format[16] = {0};   // e.g. "flac", "mp3" — file extension, lowercase from the wire
    char bitrate[16] = {0};  // e.g. "320" — kbps, string per the wire format

    uint32_t durationMs = 0;
    uint32_t positionMs = 0;
    PlayState playState = PlayState::Undefined;
    bool isPlaying = false;

    // True once at least one `track` event has ever been seen — lets
    // consumers distinguish "no MusicBee connected yet" from "stopped".
    bool hasTrack = false;

    // Set on `stop`/`offline`, cleared by the next `track` event. `offline`
    // additionally distinguishes "MusicBee closed cleanly" from an ordinary
    // stop, per NERDBOX_INTEGRATION.md's suggested "source disconnected" UI.
    bool stopped = false;
    bool offline = false;
    uint32_t stoppedAtMs = 0;  // millis() when stop/offline was received

    // Resync bookkeeping — see NERDBOX_INTEGRATION.md "Detecting missed
    // events". `session` changing means the plugin restarted (seq/trackId
    // reset legitimately, not a miss).
    uint32_t seq = 0;
    uint32_t trackId = 0;
    uint32_t session = 0;
};
