#include "WebApiHandlers.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <WiFi.h>

#include <cmath>
#include <cstring>

#include "core/ScreenRegistry.h"
#include "services/network/NetworkStatusService.h"
#include "services/web/ChunkedPrint.h"
#include "services/wifiScan/WifiScanMath.h"
#include "utils/DataFreshnessGuard.h"
#include "services/pcMetrics/DiskSnapshot.h"

namespace {
// The old snprintf-based handlers formatted these with a fixed number of
// decimal places; ArduinoJson serializes floats at full precision, so round
// explicitly to keep /api/status and /api/pc numerically stable for
// scriptable consumers.
float roundToDecimals(float value, int decimals) {
    const float scale = std::pow(10.0f, decimals);
    return std::round(value * scale) / scale;
}
}  // namespace

WebApiHandlers::WebApiHandlers(WebServer& server, ApplicationMetrics& systemMetrics,
                               PcMetrics& pcMetrics, PcMetricsService& pcMetricsService,
                               IStreamHealth& pcMetricsStreamJob, IStreamHealth& cpuClockStreamJob,
                               IStreamHealth& processStreamJob, const NetworkStatus& netStatus,
                               const SystemState& systemState, const WeatherData& weatherData,
                               const AppSettings& config,
                               const ITaskStackReporter& taskStackReporter,
                               const AudioData& audioData, const RoomClimateData& roomClimateData,
                               const NetworkManager& networkManager,
                               const WifiScanData& wifiScanData)
    : server_(server),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      pcMetricsService_(pcMetricsService),
      pcMetricsStreamJob_(pcMetricsStreamJob),
      cpuClockStreamJob_(cpuClockStreamJob),
      processStreamJob_(processStreamJob),
      netStatus_(netStatus),
      systemState_(systemState),
      weatherData_(weatherData),
      config_(config),
      taskStackReporter_(taskStackReporter),
      audioData_(audioData),
      roomClimateData_(roomClimateData),
      networkManager_(networkManager),
      wifiScanData_(wifiScanData) {}

const char* WebApiHandlers::internetStatusToString(NetworkStatus::Internet status) {
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

const char* WebApiHandlers::playStateToString(AudioData::PlayState state) {
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

const char* WebApiHandlers::screenNameToString(ScreenName screen) {
    return screenName(screen);
}

// ---------------------------------------------------------------------------
// /api/status — JSON snapshot for scriptable monitoring / the dashboard.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiStatus() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", "");

    JsonDocument doc;
    writeStatusSys(doc);
    writeStatusApp(doc);
    writeStatusNet(doc);
    writeStatusPc(doc);
    writeStatusStreams(doc);
    writeStatusUi(doc);
    writeStatusWeather(doc);
    writeStatusRoom(doc);
    writeStatusWifi(doc);
    writeStatusAudio(doc);
    writeStatusTasks(doc);

    ChunkedPrint output(server_);
    serializeJson(doc, output);
    output.flush();
    server_.sendContent("");  // flush / end chunked transfer
}

/* static */ void WebApiHandlers::writeStreamHealth(JsonObject obj, const IStreamHealth& stream) {
    obj["state"] = stream.stateName();
    obj["reconnect_count"] = stream.reconnectCount();
    obj["last_event_age_ms"] = stream.lastEventAgeMs();
    obj["overflow_count"] = stream.overflowCount();
}

void WebApiHandlers::writeStatusSys(JsonDocument& doc) {
    JsonObject sys = doc["sys"].to<JsonObject>();
    sys["heap_free"] = ESP.getFreeHeap();
    sys["heap_min"] = ESP.getMinFreeHeap();
    sys["heap_max_alloc"] = ESP.getMaxAllocHeap();
    sys["psram_size"] = ESP.getPsramSize();
    sys["psram_free"] = ESP.getFreePsram();
    sys["cpu_mhz"] = ESP.getCpuFreqMHz();
    sys["sdk"] = ESP.getSdkVersion();
    sys["reset_reason"] = static_cast<int>(esp_reset_reason());
}

void WebApiHandlers::writeStatusApp(JsonDocument& doc) {
    char uptime[20];
    systemMetrics_.getFormattedUptime(uptime, sizeof(uptime));
    JsonObject app = doc["app"].to<JsonObject>();
    app["uptime"] = uptime;
    app["uptime_ms"] = millis();
    app["parse_ms"] = systemMetrics_.getPcMetricsJsonParseTime();
    // Streaming path's counterpart to parse_ms above — the default data path
    // never fed parse_ms, leaving it permanently 0 while streaming. Reported
    // separately, in µs, so parse_ms stays meaningful for the polling fallback.
    app["stream_parse_us"] = systemMetrics_.getPcMetricsStreamParseTimeUs();
    // Draw times are measured in microseconds internally (see
    // ApplicationMetrics) — convert to ms with a decimal here for display.
    app["draw_avg_ms"] = roundToDecimals(systemMetrics_.getAverageScreenDrawTimeUs() / 1000.0f, 2);
    app["draw_max_ms"] = roundToDecimals(systemMetrics_.getMaxScreenDrawTimeUs() / 1000.0f, 2);
    app["draw_p95_ms"] = roundToDecimals(systemMetrics_.getP95ScreenDrawTimeUs() / 1000.0f, 2);
    app["widget_fps"] = roundToDecimals(systemMetrics_.getThreadWidgetFPS(), 1);

    // Coarse per-phase draw timing — only non-zero in debug builds, see
    // ThreadsWidget::onDraw.
    app["threads_bar_draw_us"] = systemMetrics_.getThreadsBarDrawTimeUs();

    // Per-widget cumulative draw-time breakdown, top few by total µs — see
    // 07-performance.md P1-22. Resets on a screen transition (each screen
    // owns its own WidgetManager), so this reflects the currently active
    // screen's widgets since it was entered, not boot-to-now.
    JsonArray widgets = app["widgets"].to<JsonArray>();
    const auto& widgetStats = systemMetrics_.getWidgetDrawStats();
    const size_t widgetStatsCount = systemMetrics_.getWidgetDrawStatsCount();
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
    app["noop_dirty_frames"] = systemMetrics_.getNoopDirtyFrames();

    JsonArray drawTimes = app["draw_times_us"].to<JsonArray>();
    const auto& screenDrawTimes = systemMetrics_.getScreenDrawTimesUs();
    const size_t drawCount = systemMetrics_.getScreenDrawCount();
    const size_t drawStart = systemMetrics_.getScreenDrawStartIndex();
    for (size_t i = 0; i < drawCount && i < screenDrawTimes.size(); ++i) {
        drawTimes.add(screenDrawTimes[(drawStart + i) % screenDrawTimes.size()]);
    }
}

void WebApiHandlers::writeStatusNet(JsonDocument& doc) {
    char ip[16] = "0.0.0.0";
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress addr = WiFi.localIP();
        snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    }
    JsonObject net = doc["net"].to<JsonObject>();
    net["wifi"] = netStatus_.wifi_connected;
    net["rssi"] = netStatus_.rssi;
    net["ip"] = ip;
    net["internet"] = internetStatusToString(netStatus_.internet);
    JsonArray probes = net["probes"].to<JsonArray>();
    for (uint8_t i = 0; i < NetworkStatusService::kNumEndpoints; ++i) {
        probes.add(netStatus_.endpoint_ok[i] ? 1 : 0);
    }
}

void WebApiHandlers::writeStatusPc(JsonDocument& doc) {
    const DataFreshnessGuard freshness(pcMetrics_.freshness);
    JsonObject pc = doc["pc"].to<JsonObject>();
    pc["available"] = pcMetrics_.freshness.available();
    pc["fresh"] = freshness.isFresh();
    pc["age_ms"] = pcMetrics_.freshness.ageMs(millis());
    pc["fetch_ok"] = pcMetricsService_.getFetchOkCount();
    pc["fetch_fail"] = pcMetricsService_.getFetchFailCount();
    pc["last_error"] = pcMetricsService_.getLastError();
}

void WebApiHandlers::writeStatusStreams(JsonDocument& doc) {
    // pc_stream — SseConnection/PcMetricsStreamJob health, so the streaming
    // path can be observed without a serial monitor. State is meaningful even
    // when pcMetricsStreamEnabled is false: it just stays DISCONNECTED since
    // the job's nextDue() is always Never. `enabled` is pc_stream-only: it's
    // the one stream with a polling fallback it can be disabled in favour of.
    JsonObject pcStream = doc["pc_stream"].to<JsonObject>();
    pcStream["enabled"] = config_.pcMetricsStreamEnabled;
    writeStreamHealth(pcStream, pcMetricsStreamJob_);

    // cpu_clock_stream / process_stream — same shape as pc_stream, for the
    // two opt-in, screen-gated streams. State is meaningful even when their
    // screen isn't active: it just stays DISCONNECTED since the job's
    // nextDue() is Never off-screen.
    writeStreamHealth(doc["cpu_clock_stream"].to<JsonObject>(), cpuClockStreamJob_);
    writeStreamHealth(doc["process_stream"].to<JsonObject>(), processStreamJob_);
}

void WebApiHandlers::writeStatusUi(JsonDocument& doc) {
    JsonObject ui = doc["ui"].to<JsonObject>();
    ui["screen"] = screenNameToString(systemState_.screen.activeScreen);
    ui["time_synced"] = systemState_.core.isTimeSynced;
}

void WebApiHandlers::writeStatusWeather(JsonDocument& doc) {
    // weather — open-meteo forecast feed freshness, so the "fetch only while
    // displayed" gating can be observed without a serial monitor.
    // refresh_pending reflects WeatherWidget's midnight-rollover signal
    // awaiting the background job.
    JsonObject weather = doc["weather"].to<JsonObject>();
    weather["available"] = weatherData_.freshness.available();
    weather["age_ms"] = weatherData_.freshness.ageMs(millis());
    weather["days"] = weatherData_.dayCount;
    weather["refresh_pending"] = weatherData_.refreshRequested.load();
}

void WebApiHandlers::writeStatusRoom(JsonDocument& doc) {
    // room — local LAN temperature/humidity sensor feed, so the room widget's
    // data path can be observed without a serial monitor — same rationale as
    // pc_stream/audio above.
    JsonObject room = doc["room"].to<JsonObject>();
    room["available"] = roomClimateData_.freshness.available();
    room["temperature_x10"] = roomClimateData_.temperature_x10;
    room["humidity"] = roomClimateData_.humidity;
    room["last_event_age_ms"] = roomClimateData_.freshness.ageMs(millis());
}

void WebApiHandlers::writeStatusWifi(JsonDocument& doc) {
    // wifi — this device's own link plus the nearby-SSID scan feed, so the
    // WIFI screen's data path can be observed without a serial monitor or a
    // visible screen. Scan fields only advance while the WIFI screen is
    // open (see docs-local/13-wifi-screen-plan.md — the job is screen-gated).
    NetworkManager::LinkInfo link{};
    networkManager_.fillLinkInfo(link);
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
    wifi["scan_state"] = static_cast<int>(wifiScanData_.state);
    wifi["scan_ap_count"] = wifiScanData_.count;
    wifi["scan_same_channel"] = wifiScanData_.sameChannelCount;
    wifi["scan_age_ms"] = wifiScanData_.freshness.ageMs(millis());
}

void WebApiHandlers::writeStatusAudio(JsonDocument& doc) {
    // audio — mb_NerdBox MusicBee plugin push feed state, so the push path
    // (POST /audio) can be observed without a serial monitor or a visible
    // screen — see docs-local/NERDBOX_INTEGRATION.md.
    JsonObject audio = doc["audio"].to<JsonObject>();
    audio["has_track"] = audioData_.hasTrack;
    audio["play_state"] = playStateToString(audioData_.playState);
    audio["is_playing"] = audioData_.isPlaying;
    audio["stopped"] = audioData_.stopped;
    audio["offline"] = audioData_.offline;
    audio["title"] = audioData_.title;
    audio["artist"] = audioData_.artist;
    audio["album"] = audioData_.album;
    audio["position_ms"] = audioData_.positionMs;
    audio["duration_ms"] = audioData_.durationMs;
    audio["seq"] = audioData_.seq;
    audio["track_id"] = audioData_.trackId;
    audio["session"] = audioData_.session;
    audio["last_event_age_ms"] = audioData_.freshness.ageMs(millis());
    // Age since the last stop/offline event, so "how long has it been
    // stopped" is observable without a screen — 0 while a track is loaded.
    audio["stopped_age_ms"] = audioData_.stopped ? millis() - audioData_.stoppedAtMs : 0;
}

void WebApiHandlers::writeStatusTasks(JsonDocument& doc) {
    // Stack high-water marks — early warning before a tight task stack
    // (6144/8192 B) overflows and reboots the device. 0 means the task
    // hasn't been created yet.
    JsonObject tasks = doc["tasks"].to<JsonObject>();
    tasks["screen_stack_free"] = taskStackReporter_.screenTaskStackFree();
    tasks["background_stack_free"] = taskStackReporter_.backgroundTaskStackFree();
}

// ---------------------------------------------------------------------------
// /api/raw — last raw NerdWinSense payload, unfiltered.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiRaw() {
    String raw;
    if (!pcMetricsService_.fetchRawJson(raw)) {
        server_.send(502, "text/plain", "Failed to fetch raw payload from NerdWinSense");
        return;
    }

    // This endpoint is a deliberate exception to the "no String churn" rule —
    // it's a rarely-hit debug page, not part of the 500 ms polling cycle, and
    // the whole point is to show the payload exactly as NerdWinSense sent it.
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", raw);
}

// ---------------------------------------------------------------------------
// /api/pc — full current PcMetrics snapshot.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiPc() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", "");

    JsonDocument doc;
    doc["available"] = pcMetrics_.freshness.available();
    doc["last_update_timestamp"] = pcMetrics_.freshness.lastUpdateMs();

#define PCM_FIELD_U(member, jsonKey) doc[jsonKey] = pcMetrics_.member;
#define PCM_FIELD_I(member, jsonKey) doc[jsonKey] = pcMetrics_.member;
#define PCM_FIELD_F2(member, jsonKey) doc[jsonKey] = roundToDecimals(pcMetrics_.member, 2);
#define PCM_FIELD(member, jsonKey, promName, help, kind) PCM_FIELD_##kind(member, jsonKey)
#include "services/pcMetrics/PcMetricsFields.def"
#undef PCM_FIELD
#undef PCM_FIELD_U
#undef PCM_FIELD_I
#undef PCM_FIELD_F2

    JsonArray threadLoad = doc["cpu_thread_load"].to<JsonArray>();
    const int threadCount =
        sizeof(pcMetrics_.cpu_thread_load) / sizeof(pcMetrics_.cpu_thread_load[0]);
    for (int i = 0; i < threadCount; ++i) {
        threadLoad.add(pcMetrics_.cpu_thread_load[i]);
    }

    JsonArray fans = doc["system_fans"].to<JsonArray>();
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count; ++i) {
        fans.add(pcMetrics_.system_fans[i]);
    }

    JsonArray drives = doc["disk_drives"].to<JsonArray>();
    DiskDrive disks[DiskSnapshot::kMaxDrives];
    const size_t diskCount = DiskSnapshot::copy(pcMetrics_, disks, DiskSnapshot::kMaxDrives);
    for (size_t i = 0; i < diskCount; ++i) {
        const DiskDrive& drive = disks[i];
        JsonObject d = drives.add<JsonObject>();
        d["name"] = drive.driveName;
        d["free_percent"] = roundToDecimals(drive.freeSpacePercent, 1);
        d["read_kbps"] = roundToDecimals(drive.readKBPerSec, 1);
        d["write_kbps"] = roundToDecimals(drive.writeKBPerSec, 1);
    }

    ChunkedPrint output(server_);
    serializeJson(doc, output);
    output.flush();
    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// /metrics — Prometheus text-exposition of the same data /api/status and
// /api/pc already aggregate, so NerdBox can be scraped directly by
// Prometheus/Grafana or polled by Home Assistant as a bonus sensor. Values
// are written straight to ChunkedPrint (a Print&) — no String assembly.
// ---------------------------------------------------------------------------

namespace {
void writeHeader(Print& out, const char* name, const char* help, const char* type) {
    out.printf("# HELP %s %s\n# TYPE %s %s\n", name, help, name, type);
}
}  // namespace

void WebApiHandlers::handleMetrics() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "text/plain; version=0.0.4; charset=utf-8", "");

    ChunkedPrint out(server_);

    // ---- system ----
    writeHeader(out, "nerdbox_uptime_seconds", "Device uptime.", "counter");
    out.printf("nerdbox_uptime_seconds %.3f\n", millis() / 1000.0);

    writeHeader(out, "nerdbox_heap_free_bytes", "Free heap memory.", "gauge");
    out.printf("nerdbox_heap_free_bytes %u\n", ESP.getFreeHeap());

    writeHeader(out, "nerdbox_psram_free_bytes", "Free PSRAM.", "gauge");
    out.printf("nerdbox_psram_free_bytes %u\n", ESP.getFreePsram());

    // ---- app ----
    writeHeader(out, "nerdbox_draw_time_avg_ms", "Average screen draw time.", "gauge");
    out.printf("nerdbox_draw_time_avg_ms %.3f\n",
               systemMetrics_.getAverageScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_draw_time_max_ms", "Max screen draw time in the retained window.",
                "gauge");
    out.printf("nerdbox_draw_time_max_ms %.3f\n", systemMetrics_.getMaxScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_draw_time_p95_ms", "P95 screen draw time in the retained window.",
                "gauge");
    out.printf("nerdbox_draw_time_p95_ms %.3f\n", systemMetrics_.getP95ScreenDrawTimeUs() / 1000.0);

    writeHeader(out, "nerdbox_widget_fps", "Threads widget redraw rate.", "gauge");
    out.printf("nerdbox_widget_fps %.2f\n", systemMetrics_.getThreadWidgetFPS());

    // ---- network ----
    writeHeader(out, "nerdbox_wifi_connected", "WiFi connection state (1=connected).", "gauge");
    out.printf("nerdbox_wifi_connected %d\n", netStatus_.wifi_connected ? 1 : 0);

    writeHeader(out, "nerdbox_wifi_rssi_dbm", "WiFi signal strength.", "gauge");
    out.printf("nerdbox_wifi_rssi_dbm %d\n", netStatus_.rssi);

    writeHeader(out, "nerdbox_internet_reachable", "Internet-reachability probe state (1=OK).",
                "gauge");
    out.printf("nerdbox_internet_reachable %d\n",
               netStatus_.internet == NetworkStatus::Internet::OK ? 1 : 0);

    // ---- pc-metrics feed health ----
    const DataFreshnessGuard freshness(pcMetrics_.freshness);
    writeHeader(out, "nerdbox_pc_available", "Whether a PC-metrics reading has ever been received.",
                "gauge");
    out.printf("nerdbox_pc_available %d\n", pcMetrics_.freshness.available() ? 1 : 0);

    writeHeader(out, "nerdbox_pc_fresh",
                "Whether the last PC-metrics reading is within the staleness window.", "gauge");
    out.printf("nerdbox_pc_fresh %d\n", freshness.isFresh() ? 1 : 0);

    // ---- SSE stream health — all three streams, labelled by stream, so a
    // flapping opt-in stream is as visible here as in /api/status. ----
    struct NamedStream {
        const char* label;
        const IStreamHealth& health;
    };
    const NamedStream streams[] = {{"pc", pcMetricsStreamJob_},
                                   {"cpu_clock", cpuClockStreamJob_},
                                   {"process", processStreamJob_}};
    writeHeader(out, "nerdbox_stream_reconnects_total", "SSE reconnect count since boot.",
                "counter");
    for (const NamedStream& st : streams) {
        out.printf("nerdbox_stream_reconnects_total{stream=\"%s\"} %lu\n", st.label,
                   static_cast<unsigned long>(st.health.reconnectCount()));
    }
    writeHeader(out, "nerdbox_stream_overflow_total",
                "SSE event-buffer overflow count since boot.", "counter");
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
    out.printf(promName " %u\n", pcMetrics_.member);
#define PCM_FIELD_I(member, promName, help)    \
    writeHeader(out, promName, help, "gauge"); \
    out.printf(promName " %d\n", pcMetrics_.member);
#define PCM_FIELD_F2(member, promName, help)   \
    writeHeader(out, promName, help, "gauge"); \
    out.printf(promName " %.2f\n", pcMetrics_.member);
#define PCM_FIELD(member, jsonKey, promName, help, kind) PCM_FIELD_##kind(member, promName, help)
#include "services/pcMetrics/PcMetricsFields.def"
#undef PCM_FIELD
#undef PCM_FIELD_U
#undef PCM_FIELD_I
#undef PCM_FIELD_F2

    writeHeader(out, "nerdbox_pc_cpu_thread_load_percent", "Per-thread CPU load.", "gauge");
    const int threadCount =
        sizeof(pcMetrics_.cpu_thread_load) / sizeof(pcMetrics_.cpu_thread_load[0]);
    for (int i = 0; i < threadCount; ++i) {
        out.printf("nerdbox_pc_cpu_thread_load_percent{thread=\"%d\"} %u\n", i,
                   pcMetrics_.cpu_thread_load[i]);
    }

    writeHeader(out, "nerdbox_pc_system_fan_rpm", "Motherboard-header fan speeds.", "gauge");
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count; ++i) {
        out.printf("nerdbox_pc_system_fan_rpm{fan=\"%u\"} %u\n", i, pcMetrics_.system_fans[i]);
    }

    // disk_drives is written from core 0 — DiskSnapshot::copy() snapshots it
    // under the lock so the printing below runs with it released.
    DiskDrive disks[DiskSnapshot::kMaxDrives];
    const size_t diskCount = DiskSnapshot::copy(pcMetrics_, disks, DiskSnapshot::kMaxDrives);

    writeHeader(out, "nerdbox_pc_disk_free_percent", "Free space per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_free_percent{drive=\"%s\"} %.1f\n", disks[i].driveName,
                   disks[i].freeSpacePercent);
    }
    writeHeader(out, "nerdbox_pc_disk_read_kbps", "Read throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_read_kbps{drive=\"%s\"} %.1f\n", disks[i].driveName,
                   disks[i].readKBPerSec);
    }
    writeHeader(out, "nerdbox_pc_disk_write_kbps", "Write throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_write_kbps{drive=\"%s\"} %.1f\n", disks[i].driveName,
                   disks[i].writeKBPerSec);
    }

    // ---- weather feed health ----
    writeHeader(out, "nerdbox_weather_available", "Whether a weather forecast has been fetched.",
                "gauge");
    out.printf("nerdbox_weather_available %d\n", weatherData_.freshness.available() ? 1 : 0);

    // ---- room climate (local LAN sensor) ----
    writeHeader(out, "nerdbox_room_temperature_celsius", "Local room temperature.", "gauge");
    out.printf("nerdbox_room_temperature_celsius %.1f\n", roomClimateData_.temperature_x10 / 10.0);

    writeHeader(out, "nerdbox_room_humidity_percent", "Local room relative humidity.", "gauge");
    out.printf("nerdbox_room_humidity_percent %u\n", roomClimateData_.humidity);

    // ---- this device's own WiFi link ----
    NetworkManager::LinkInfo link{};
    networkManager_.fillLinkInfo(link);

    writeHeader(out, "nerdbox_wifi_rssi_dbm", "This device's own WiFi signal strength.", "gauge");
    out.printf("nerdbox_wifi_rssi_dbm %d\n", link.rssi);

    writeHeader(out, "nerdbox_wifi_quality_percent", "This device's own WiFi signal quality.",
               "gauge");
    out.printf("nerdbox_wifi_quality_percent %u\n", WifiScanMath::rssiToQuality(link.rssi));

    writeHeader(out, "nerdbox_wifi_channel", "This device's own WiFi channel.", "gauge");
    out.printf("nerdbox_wifi_channel %u\n", link.channel);

    writeHeader(out, "nerdbox_wifi_neighbours_same_channel",
               "Nearby APs sharing this device's WiFi channel (last scan).", "gauge");
    out.printf("nerdbox_wifi_neighbours_same_channel %u\n", wifiScanData_.sameChannelCount);

    out.flush();
    server_.sendContent("");  // flush / end chunked transfer
}
