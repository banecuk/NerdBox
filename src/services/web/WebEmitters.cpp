#include "services/web/WebEmitters.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "config/Limits.h"
#include "core/ScreenRegistry.h"
#include "services/pcMetrics/DiskSnapshot.h"
#include "services/wifiScan/WifiScanMath.h"

// Pure emitters behind /api/status, /api/pc and /metrics — no WebServer, ESP
// or WiFi dependency, so this file also builds under [env:native]
// (test/MetricsFormatTest.cpp). Hardware/clock reads happen in
// WebApiHandlers::captureSnapshot() before any of this runs.

namespace {

// The old snprintf-based handlers formatted these with a fixed number of
// decimal places; ArduinoJson serializes floats at full precision, so round
// explicitly to keep /api/status and /api/pc numerically stable for
// scriptable consumers.
float roundToDecimals(float value, int decimals) {
    const float scale = std::pow(10.0f, decimals);
    return std::round(value * scale) / scale;
}

const char* internetStatusToString(NetworkStatus::Internet status) {
    switch (status) {
        case NetworkStatus::Internet::OK:
            return "OK";
        case NetworkStatus::Internet::WARNING:
            return "WARNING";
        case NetworkStatus::Internet::DEGRADED:
            return "DEGRADED";
        case NetworkStatus::Internet::DOWN:
            return "DOWN";
        case NetworkStatus::Internet::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

const char* playStateToString(AudioData::PlayState state) {
    switch (state) {
        case AudioData::PlayState::Loading:
            return "LOADING";
        case AudioData::PlayState::Playing:
            return "PLAYING";
        case AudioData::PlayState::Paused:
            return "PAUSED";
        case AudioData::PlayState::Stopped:
            return "STOPPED";
        case AudioData::PlayState::Undefined:
        default:
            return "UNDEFINED";
    }
}

// Valid entries of cpu_thread_load — cpu_core_count, clamped to the backing
// array, not the whole kMaxThreads-sized array (B65: a 28-thread PC exported
// 20 phantom always-0 % threads).
int threadCount(const PcMetrics& pc) {
    return std::min<int>(pc.cpu_core_count, AppConfig::Limits::kMaxThreads);
}

// ---------------------------------------------------------------------------
// /api/status — one independent object per feed; each writer touches only
// the members it names.
// ---------------------------------------------------------------------------

// Shared shape of pc_stream/cpu_clock_stream/process_stream, so the three
// objects can't drift apart.
void writeStreamHealth(JsonObject obj, const IStreamHealth& stream) {
    obj["state"] = stream.stateName();
    obj["reconnect_count"] = stream.reconnectCount();
    obj["last_event_age_ms"] = stream.lastEventAgeMs();
    obj["overflow_count"] = stream.overflowCount();
}

void writeStatusSys(JsonDocument& doc, const StatusSnapshot& snap) {
    JsonObject sys = doc["sys"].to<JsonObject>();
    sys["heap_free"] = snap.heapFree;
    sys["heap_min"] = snap.heapMin;
    sys["heap_max_alloc"] = snap.heapMaxAlloc;
    sys["psram_size"] = snap.psramSize;
    sys["psram_free"] = snap.psramFree;
    sys["cpu_mhz"] = snap.cpuMhz;
    sys["sdk"] = snap.sdk;
    sys["reset_reason"] = snap.resetReason;
}

void writeStatusApp(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    const ApplicationMetrics& metrics = feeds.systemMetrics;
    JsonObject app = doc["app"].to<JsonObject>();
    app["uptime"] = snap.uptime;
    app["uptime_ms"] = snap.uptimeMs;  // 64-bit, no 49.7-day wrap (B59)
    app["parse_ms"] = metrics.getPcMetricsJsonParseTime();
    // Streaming path's counterpart to parse_ms above — the default data path
    // never fed parse_ms, leaving it permanently 0 while streaming. Reported
    // separately, in µs, so parse_ms stays meaningful for the polling fallback.
    app["stream_parse_us"] = metrics.getPcMetricsStreamParseTimeUs();
    // Draw times are measured in microseconds internally (see
    // ApplicationMetrics) — convert to ms with a decimal here for display.
    app["draw_avg_ms"] = roundToDecimals(metrics.getAverageScreenDrawTimeUs() / 1000.0f, 2);
    app["draw_max_ms"] = roundToDecimals(metrics.getMaxScreenDrawTimeUs() / 1000.0f, 2);
    app["draw_p95_ms"] = roundToDecimals(metrics.getP95ScreenDrawTimeUs() / 1000.0f, 2);
    app["widget_fps"] = roundToDecimals(metrics.getThreadWidgetFPS(), 1);

    // Coarse per-phase draw timing — only non-zero in debug builds, see
    // ThreadsWidget::onDraw.
    app["threads_bar_draw_us"] = metrics.getThreadsBarDrawTimeUs();

    // Per-widget cumulative draw-time breakdown, top few by total µs — see
    // 07-performance.md P1-22. Resets on a screen transition (each screen
    // owns its own WidgetManager), so this reflects the currently active
    // screen's widgets since it was entered, not boot-to-now.
    JsonArray widgets = app["widgets"].to<JsonArray>();
    const auto& widgetStats = metrics.getWidgetDrawStats();
    const size_t widgetStatsCount = metrics.getWidgetDrawStatsCount();
    for (size_t i = 0; i < widgetStatsCount; ++i) {
        JsonObject w = widgets.add<JsonObject>();
        w["label"] = widgetStats[i].label;
        w["calls"] = widgetStats[i].calls;
        w["total_us"] = widgetStats[i].totalUs;
    }
    // Frames where a widget was found dirty but nothing ended up drawn — see
    // ApplicationMetrics::incrementNoopDirtyFrames()'s comment; P1-20 was
    // exactly this shape and would have shown up here as a steadily
    // incrementing counter instead of needing to be found by reading code.
    app["noop_dirty_frames"] = metrics.getNoopDirtyFrames();

    JsonArray drawTimes = app["draw_times_us"].to<JsonArray>();
    const auto& screenDrawTimes = metrics.getScreenDrawTimesUs();
    const size_t drawCount = metrics.getScreenDrawCount();
    const size_t drawStart = metrics.getScreenDrawStartIndex();
    for (size_t i = 0; i < drawCount && i < screenDrawTimes.size(); ++i) {
        drawTimes.add(screenDrawTimes[(drawStart + i) % screenDrawTimes.size()]);
    }
}

void writeStatusNet(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    const NetworkStatus& netStatus = feeds.netStatus;
    JsonObject net = doc["net"].to<JsonObject>();
    net["wifi"] = netStatus.wifi_connected;
    net["rssi"] = netStatus.rssi;
    net["ip"] = snap.netIp;
    net["internet"] = internetStatusToString(netStatus.internet);
    JsonArray probes = net["probes"].to<JsonArray>();
    for (uint8_t i = 0; i < AppConfig::Limits::kNetworkProbeEndpoints; ++i) {
        probes.add(netStatus.endpoint_ok[i] ? 1 : 0);
    }
}

void writeStatusPc(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    JsonObject pc = doc["pc"].to<JsonObject>();
    pc["available"] = feeds.pcMetrics.freshness.available();
    pc["fresh"] = snap.pcFresh;
    pc["age_ms"] = feeds.pcMetrics.freshness.ageMs(snap.nowMs);
    pc["fetch_ok"] = snap.pcFetchOk;
    pc["fetch_fail"] = snap.pcFetchFail;
    pc["last_error"] = snap.pcLastError;
}

void writeStatusStreams(JsonDocument& doc, const WebFeeds& feeds) {
    // pc_stream — SseConnection/PcMetricsStreamJob health, so the streaming
    // path can be observed without a serial monitor. State is meaningful even
    // when pcMetricsStreamEnabled is false: it just stays DISCONNECTED since
    // the job's nextDue() is always Never. `enabled` is pc_stream-only: it's
    // the one stream with a polling fallback it can be disabled in favour of.
    JsonObject pcStream = doc["pc_stream"].to<JsonObject>();
    pcStream["enabled"] = feeds.config.pcMetricsStreamEnabled;
    writeStreamHealth(pcStream, feeds.pcMetricsStream);

    // cpu_clock_stream / process_stream — same shape as pc_stream, for the
    // two opt-in, screen-gated streams. State is meaningful even when their
    // screen isn't active: it just stays DISCONNECTED since the job's
    // nextDue() is Never off-screen.
    writeStreamHealth(doc["cpu_clock_stream"].to<JsonObject>(), feeds.cpuClockStream);
    writeStreamHealth(doc["process_stream"].to<JsonObject>(), feeds.processStream);
}

void writeStatusUi(JsonDocument& doc, const WebFeeds& feeds) {
    JsonObject ui = doc["ui"].to<JsonObject>();
    ui["screen"] = screenName(feeds.systemState.screen.activeScreen.load());
    ui["time_synced"] = feeds.systemState.core.isTimeSynced.load();
}

void writeStatusWeather(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    // weather — open-meteo forecast feed freshness, observable without a
    // serial monitor. refresh_pending is a screen-entry or midnight-rollover
    // refetch request (WeatherScreen::onEnter() / WeatherJob) awaiting the
    // background job.
    const WeatherData& weatherData = feeds.weatherData;
    JsonObject weather = doc["weather"].to<JsonObject>();
    weather["available"] = weatherData.freshness.available();
    weather["age_ms"] = weatherData.freshness.ageMs(snap.nowMs);
    weather["days"] = weatherData.dayCount;
    weather["refresh_pending"] = weatherData.refreshRequested.load();
}

void writeStatusRoom(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    // room — local LAN temperature/humidity sensor feed, so the room widget's
    // data path can be observed without a serial monitor — same rationale as
    // pc_stream/audio above.
    const RoomClimateData& roomClimate = feeds.roomClimateData;
    JsonObject room = doc["room"].to<JsonObject>();
    room["available"] = roomClimate.freshness.available();
    room["temperature_x10"] = roomClimate.temperature_x10;
    room["humidity"] = roomClimate.humidity;
    room["last_event_age_ms"] = roomClimate.freshness.ageMs(snap.nowMs);
}

void writeStatusWifi(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    // wifi — this device's own link plus the nearby-SSID scan feed, so the
    // WIFI screen's data path can be observed without a serial monitor or a
    // visible screen. Scan fields only advance while the WIFI screen is
    // open (see docs-local/13-wifi-screen-plan.md — the job is screen-gated).
    const LinkInfo& link = snap.link;
    const WifiScanData& scan = feeds.wifiScanData;
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = link.connected;
    wifi["ssid"] = link.ssid;
    wifi["rssi"] = link.rssi;
    wifi["quality"] = WifiScanMath::rssiToQuality(link.rssi);
    wifi["channel"] = link.channel;
    char bssidStr[18] = "";
    snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X", link.bssid[0],
             link.bssid[1], link.bssid[2], link.bssid[3], link.bssid[4], link.bssid[5]);
    wifi["bssid"] = bssidStr;
    wifi["auth"] = WifiScanMath::authName(link.auth);
    wifi["ip"] = link.ip;
    wifi["gateway"] = link.gateway;
    wifi["mac"] = link.mac;
    wifi["hostname"] = link.hostname;
    wifi["scan_state"] = static_cast<int>(scan.state);
    wifi["scan_ap_count"] = scan.count;
    wifi["scan_same_channel"] = scan.sameChannelCount;
    wifi["scan_age_ms"] = scan.freshness.ageMs(snap.nowMs);
}

void writeStatusAudio(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    // audio — mb_NerdBox MusicBee plugin push feed state, so the push path
    // (POST /audio) can be observed without a serial monitor or a visible
    // screen — see docs-local/NERDBOX_INTEGRATION.md.
    const AudioData& audioData = feeds.audioData;
    JsonObject audio = doc["audio"].to<JsonObject>();
    audio["has_track"] = audioData.hasTrack;
    audio["play_state"] = playStateToString(audioData.playState);
    audio["is_playing"] = audioData.isPlaying;
    audio["stopped"] = audioData.stopped;
    audio["offline"] = audioData.offline;
    audio["title"] = audioData.title;
    audio["artist"] = audioData.artist;
    audio["album"] = audioData.album;
    audio["position_ms"] = audioData.positionMs;
    audio["duration_ms"] = audioData.durationMs;
    audio["seq"] = audioData.seq;
    audio["track_id"] = audioData.trackId;
    audio["session"] = audioData.session;
    audio["last_event_age_ms"] = audioData.freshness.ageMs(snap.nowMs);
    // Age since the last stop/offline event, so "how long has it been
    // stopped" is observable without a screen — 0 while a track is loaded.
    audio["stopped_age_ms"] = audioData.stopped ? snap.nowMs - audioData.stoppedAtMs : 0;
}

void writeStatusTasks(JsonDocument& doc, const WebFeeds& feeds) {
    // Stack high-water marks — early warning before a tight task stack
    // (6144/8192 B) overflows and reboots the device. 0 means the task
    // hasn't been created yet.
    JsonObject tasks = doc["tasks"].to<JsonObject>();
    tasks["screen_stack_free"] = feeds.taskStacks.screenTaskStackFree();
    tasks["background_stack_free"] = feeds.taskStacks.backgroundTaskStackFree();
    tasks["watchdog_active"] = feeds.taskStacks.watchdogActive();
}

// ---------------------------------------------------------------------------
// /metrics helpers
// ---------------------------------------------------------------------------

void writeHeader(Print& out, const char* name, const char* help, const char* type) {
    out.printf("# HELP %s %s\n# TYPE %s %s\n", name, help, name, type);
}

}  // namespace

namespace WebEmitters {

void writeStatus(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds) {
    writeStatusSys(doc, snap);
    writeStatusApp(doc, snap, feeds);
    writeStatusNet(doc, snap, feeds);
    writeStatusPc(doc, snap, feeds);
    writeStatusStreams(doc, feeds);
    writeStatusUi(doc, feeds);
    writeStatusWeather(doc, snap, feeds);
    writeStatusRoom(doc, snap, feeds);
    writeStatusWifi(doc, snap, feeds);
    writeStatusAudio(doc, snap, feeds);
    writeStatusTasks(doc, feeds);
}

// ---------------------------------------------------------------------------
// /api/pc — full current PcMetrics snapshot.
// ---------------------------------------------------------------------------

void writePc(JsonDocument& doc, const WebFeeds& feeds) {
    const PcMetrics& pcMetrics = feeds.pcMetrics;
    doc["available"] = pcMetrics.freshness.available();
    doc["last_update_timestamp"] = pcMetrics.freshness.lastUpdateMs();

#define PCM_FIELD_U(member, jsonKey) doc[jsonKey] = pcMetrics.member;
#define PCM_FIELD_I(member, jsonKey) doc[jsonKey] = pcMetrics.member;
#define PCM_FIELD_F2(member, jsonKey) doc[jsonKey] = roundToDecimals(pcMetrics.member, 2);
#define PCM_FIELD(member, jsonKey, promName, help, kind) PCM_FIELD_##kind(member, jsonKey)
#include "services/pcMetrics/PcMetricsFields.def"
#undef PCM_FIELD
#undef PCM_FIELD_U
#undef PCM_FIELD_I
#undef PCM_FIELD_F2

    JsonArray threadLoad = doc["cpu_thread_load"].to<JsonArray>();
    const int threads = threadCount(pcMetrics);
    for (int i = 0; i < threads; ++i) {
        threadLoad.add(pcMetrics.cpu_thread_load[i]);
    }

    JsonArray fans = doc["system_fans"].to<JsonArray>();
    for (uint8_t i = 0; i < pcMetrics.system_fan_count; ++i) {
        fans.add(pcMetrics.system_fans[i]);
    }

    JsonArray drives = doc["disk_drives"].to<JsonArray>();
    DiskDrive disks[DiskSnapshot::kMaxDrives];
    const size_t diskCount = DiskSnapshot::copy(pcMetrics, disks, DiskSnapshot::kMaxDrives);
    for (size_t i = 0; i < diskCount; ++i) {
        const DiskDrive& drive = disks[i];
        JsonObject d = drives.add<JsonObject>();
        d["name"] = drive.driveName;
        d["free_percent"] = roundToDecimals(drive.freeSpacePercent, 1);
        d["read_kbps"] = roundToDecimals(drive.readKBPerSec, 1);
        d["write_kbps"] = roundToDecimals(drive.writeKBPerSec, 1);
    }
}

// ---------------------------------------------------------------------------
// /metrics — Prometheus text-exposition of the same data /api/status and
// /api/pc already aggregate, so NerdBox can be scraped directly by
// Prometheus/Grafana or polled by Home Assistant as a bonus sensor. Values
// are written straight to a Print& — no String assembly.
// ---------------------------------------------------------------------------

void writeMetrics(Print& out, const StatusSnapshot& snap, const WebFeeds& feeds) {
    const ApplicationMetrics& systemMetrics = feeds.systemMetrics;
    const NetworkStatus& netStatus = feeds.netStatus;
    const PcMetrics& pcMetrics = feeds.pcMetrics;

    // ---- system ----
    writeHeader(out, "nerdbox_uptime_seconds", "Device uptime.", "counter");
    // 64-bit clock: a counter that resets at the millis() wrap reads as a
    // reboot to Prometheus (B59).
    out.printf("nerdbox_uptime_seconds %.3f\n", static_cast<double>(snap.uptimeMs) / 1000.0);

    writeHeader(out, "nerdbox_heap_free_bytes", "Free heap memory.", "gauge");
    out.printf("nerdbox_heap_free_bytes %u\n", static_cast<unsigned>(snap.heapFree));

    writeHeader(out, "nerdbox_psram_free_bytes", "Free PSRAM.", "gauge");
    out.printf("nerdbox_psram_free_bytes %u\n", static_cast<unsigned>(snap.psramFree));

    // ---- app ----
    writeHeader(out, "nerdbox_draw_time_avg_ms", "Average screen draw time.", "gauge");
    out.printf("nerdbox_draw_time_avg_ms %.3f\n",
               systemMetrics.getAverageScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_draw_time_max_ms", "Max screen draw time in the retained window.",
                "gauge");
    out.printf("nerdbox_draw_time_max_ms %.3f\n", systemMetrics.getMaxScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_draw_time_p95_ms", "P95 screen draw time in the retained window.",
                "gauge");
    out.printf("nerdbox_draw_time_p95_ms %.3f\n", systemMetrics.getP95ScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_widget_fps", "Threads widget redraw rate.", "gauge");
    out.printf("nerdbox_widget_fps %.2f\n",
               static_cast<double>(systemMetrics.getThreadWidgetFPS()));

    // ---- network ----
    writeHeader(out, "nerdbox_wifi_connected", "WiFi connection state (1=connected).", "gauge");
    out.printf("nerdbox_wifi_connected %d\n", netStatus.wifi_connected ? 1 : 0);

    writeHeader(out, "nerdbox_wifi_rssi_dbm", "WiFi signal strength.", "gauge");
    out.printf("nerdbox_wifi_rssi_dbm %d\n", netStatus.rssi);

    writeHeader(out, "nerdbox_internet_reachable", "Internet-reachability probe state (1=OK).",
                "gauge");
    out.printf("nerdbox_internet_reachable %d\n",
               netStatus.internet == NetworkStatus::Internet::OK ? 1 : 0);

    // ---- pc-metrics feed health ----
    writeHeader(out, "nerdbox_pc_available", "Whether a PC-metrics reading has ever been received.",
                "gauge");
    out.printf("nerdbox_pc_available %d\n", pcMetrics.freshness.available() ? 1 : 0);

    writeHeader(out, "nerdbox_pc_fresh",
                "Whether the last PC-metrics reading is within the staleness window.", "gauge");
    out.printf("nerdbox_pc_fresh %d\n", snap.pcFresh ? 1 : 0);

    // ---- SSE stream health — all three streams, labelled by stream, so a
    // flapping opt-in stream is as visible here as in /api/status. ----
    struct NamedStream {
        const char* label;
        const IStreamHealth& health;
    };
    const NamedStream streams[] = {
        {"pc",        feeds.pcMetricsStream},
        {"cpu_clock", feeds.cpuClockStream },
        {"process",   feeds.processStream  }
    };
    writeHeader(out, "nerdbox_stream_reconnects_total", "SSE reconnect count since boot.",
                "counter");
    for (const NamedStream& st : streams) {
        out.printf("nerdbox_stream_reconnects_total{stream=\"%s\"} %lu\n", st.label,
                   static_cast<unsigned long>(st.health.reconnectCount()));
    }
    writeHeader(out, "nerdbox_stream_overflow_total", "SSE event-buffer overflow count since boot.",
                "counter");
    for (const NamedStream& st : streams) {
        out.printf("nerdbox_stream_overflow_total{stream=\"%s\"} %lu\n", st.label,
                   static_cast<unsigned long>(st.health.overflowCount()));
    }
    writeHeader(out, "nerdbox_stream_last_event_age_ms",
                "Milliseconds since the last SSE event was received.", "gauge");
    for (const NamedStream& st : streams) {
        out.printf("nerdbox_stream_last_event_age_ms{stream=\"%s\"} %lu\n", st.label,
                   static_cast<unsigned long>(st.health.lastEventAgeMs()));
    }

    // ---- pc metrics values ----
#define PCM_FIELD_U(member, promName, help)    \
    writeHeader(out, promName, help, "gauge"); \
    out.printf(promName " %u\n", static_cast<unsigned>(pcMetrics.member));
#define PCM_FIELD_I(member, promName, help)    \
    writeHeader(out, promName, help, "gauge"); \
    out.printf(promName " %d\n", static_cast<int>(pcMetrics.member));
#define PCM_FIELD_F2(member, promName, help)   \
    writeHeader(out, promName, help, "gauge"); \
    out.printf(promName " %.2f\n", static_cast<double>(pcMetrics.member));
#define PCM_FIELD(member, jsonKey, promName, help, kind) PCM_FIELD_##kind(member, promName, help)
#include "services/pcMetrics/PcMetricsFields.def"
#undef PCM_FIELD
#undef PCM_FIELD_U
#undef PCM_FIELD_I
#undef PCM_FIELD_F2

    writeHeader(out, "nerdbox_pc_cpu_thread_load_percent", "Per-thread CPU load.", "gauge");
    const int threads = threadCount(pcMetrics);
    for (int i = 0; i < threads; ++i) {
        out.printf("nerdbox_pc_cpu_thread_load_percent{thread=\"%d\"} %u\n", i,
                   static_cast<unsigned>(pcMetrics.cpu_thread_load[i]));
    }

    writeHeader(out, "nerdbox_pc_system_fan_rpm", "Motherboard-header fan speeds.", "gauge");
    for (uint8_t i = 0; i < pcMetrics.system_fan_count; ++i) {
        out.printf("nerdbox_pc_system_fan_rpm{fan=\"%u\"} %u\n", static_cast<unsigned>(i),
                   static_cast<unsigned>(pcMetrics.system_fans[i]));
    }

    // disk_drives is written from core 0 — DiskSnapshot::copy() snapshots it
    // under the lock so the printing below runs with it released. Drive
    // names come from the PC, so they're escaped before going into a label
    // value — a raw `"` or `\` breaks the whole scrape for strict parsers (B66).
    DiskDrive disks[DiskSnapshot::kMaxDrives];
    const size_t diskCount = DiskSnapshot::copy(pcMetrics, disks, DiskSnapshot::kMaxDrives);
    char driveLabels[DiskSnapshot::kMaxDrives][2 * sizeof(DiskDrive::driveName) + 1];
    for (size_t i = 0; i < diskCount; ++i) {
        escapeLabelValue(disks[i].driveName, driveLabels[i], sizeof(driveLabels[i]));
    }

    writeHeader(out, "nerdbox_pc_disk_free_percent", "Free space per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_free_percent{drive=\"%s\"} %.1f\n", driveLabels[i],
                   static_cast<double>(disks[i].freeSpacePercent));
    }
    writeHeader(out, "nerdbox_pc_disk_read_kbps", "Read throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_read_kbps{drive=\"%s\"} %.1f\n", driveLabels[i],
                   static_cast<double>(disks[i].readKBPerSec));
    }
    writeHeader(out, "nerdbox_pc_disk_write_kbps", "Write throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_write_kbps{drive=\"%s\"} %.1f\n", driveLabels[i],
                   static_cast<double>(disks[i].writeKBPerSec));
    }

    // ---- weather feed health ----
    writeHeader(out, "nerdbox_weather_available", "Whether a weather forecast has been fetched.",
                "gauge");
    out.printf("nerdbox_weather_available %d\n", feeds.weatherData.freshness.available() ? 1 : 0);

    // ---- room climate (local LAN sensor) ----
    // No sample until the sensor has been read once: a never-reached sensor
    // used to export a real-looking 0.0 °C / 0 % series (B66).
    const RoomClimateData& roomClimate = feeds.roomClimateData;
    const bool roomAvailable = roomClimate.freshness.available();
    writeHeader(out, "nerdbox_room_temperature_celsius", "Local room temperature.", "gauge");
    if (roomAvailable) {
        out.printf("nerdbox_room_temperature_celsius %.1f\n", roomClimate.temperature_x10 / 10.0);
    }

    writeHeader(out, "nerdbox_room_humidity_percent", "Local room relative humidity.", "gauge");
    if (roomAvailable) {
        out.printf("nerdbox_room_humidity_percent %u\n",
                   static_cast<unsigned>(roomClimate.humidity));
    }

    // ---- this device's own WiFi link ----
    // nerdbox_wifi_rssi_dbm is already emitted above (from NetworkStatus) —
    // a second HELP/TYPE block for the same name makes strict parsers
    // (promtool, Go expfmt, Telegraf) reject the whole scrape (B45).
    // Quality/channel have no meaning while disconnected, so no sample then
    // (B66) — nerdbox_wifi_connected says why.
    const LinkInfo& link = snap.link;
    writeHeader(out, "nerdbox_wifi_quality_percent", "This device's own WiFi signal quality.",
                "gauge");
    if (link.connected) {
        out.printf("nerdbox_wifi_quality_percent %u\n",
                   static_cast<unsigned>(WifiScanMath::rssiToQuality(link.rssi)));
    }

    writeHeader(out, "nerdbox_wifi_channel", "This device's own WiFi channel.", "gauge");
    if (link.connected) {
        out.printf("nerdbox_wifi_channel %u\n", static_cast<unsigned>(link.channel));
    }

    writeHeader(out, "nerdbox_wifi_neighbours_same_channel",
                "Nearby APs sharing this device's WiFi channel (last scan).", "gauge");
    out.printf("nerdbox_wifi_neighbours_same_channel %u\n",
               static_cast<unsigned>(feeds.wifiScanData.sameChannelCount));
}

size_t escapeLabelValue(const char* in, char* out, size_t outSize) {
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    size_t n = 0;
    for (const char* p = in; p != nullptr && *p != '\0'; ++p) {
        char escaped = 0;
        switch (*p) {
            case '\\':
                escaped = '\\';
                break;
            case '"':
                escaped = '"';
                break;
            case '\n':
                escaped = 'n';
                break;
            default:
                break;
        }
        const size_t need = escaped ? 2 : 1;
        if (n + need >= outSize) {
            break;  // never split an escape sequence in half
        }
        if (escaped) {
            out[n++] = '\\';
            out[n++] = escaped;
        } else {
            out[n++] = *p;
        }
    }
    out[n] = '\0';
    return n;
}

}  // namespace WebEmitters
