#include "WebApiHandlers.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <WiFi.h>

#include <cmath>
#include <cstring>

#include "services/network/NetworkStatusService.h"
#include "services/web/ChunkedPrint.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/ScopedLock.h"

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
                               SseStreamJob& pcMetricsStreamJob, SseStreamJob& cpuClockStreamJob,
                               SseStreamJob& processStreamJob, const NetworkStatus& netStatus,
                               const SystemState& systemState, const WeatherData& weatherData,
                               const AppSettings& config, const TaskManager& taskManager,
                               const AudioData& audioData, const RoomClimateData& roomClimateData)
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
      taskManager_(taskManager),
      audioData_(audioData),
      roomClimateData_(roomClimateData) {}

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

const char* WebApiHandlers::sseStateToString(SseConnection::State state) {
    switch (state) {
        case SseConnection::State::Disconnected:
            return "DISCONNECTED";
        case SseConnection::State::Connected:
            return "CONNECTED";
        case SseConnection::State::Error:
            return "ERROR";
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
    switch (screen) {
        case ScreenName::BOOT:
            return "BOOT";
        case ScreenName::MAIN:
            return "MAIN";
        case ScreenName::SETTINGS:
            return "SETTINGS";
        case ScreenName::GAME:
            return "GAME";
        case ScreenName::DISKS:
            return "DISKS";
        case ScreenName::WEATHER:
            return "WEATHER";
        case ScreenName::CALENDAR:
            return "CALENDAR";
        case ScreenName::NONE:
        default:
            return "NONE";
    }
}

// ---------------------------------------------------------------------------
// /api/status — JSON snapshot for scriptable monitoring / the dashboard.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiStatus() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", "");

    JsonDocument doc;

    JsonObject sys = doc["sys"].to<JsonObject>();
    sys["heap_free"] = ESP.getFreeHeap();
    sys["heap_min"] = ESP.getMinFreeHeap();
    sys["heap_max_alloc"] = ESP.getMaxAllocHeap();
    sys["psram_size"] = ESP.getPsramSize();
    sys["psram_free"] = ESP.getFreePsram();
    sys["cpu_mhz"] = ESP.getCpuFreqMHz();
    sys["sdk"] = ESP.getSdkVersion();
    sys["reset_reason"] = static_cast<int>(esp_reset_reason());

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

    JsonArray drawTimes = app["draw_times_us"].to<JsonArray>();
    const auto& screenDrawTimes = systemMetrics_.getScreenDrawTimesUs();
    const size_t drawCount = systemMetrics_.getScreenDrawCount();
    const size_t drawStart = systemMetrics_.getScreenDrawStartIndex();
    for (size_t i = 0; i < drawCount && i < screenDrawTimes.size(); ++i) {
        drawTimes.add(screenDrawTimes[(drawStart + i) % screenDrawTimes.size()]);
    }

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

    const DataFreshnessGuard freshness(pcMetrics_.freshness);
    const unsigned long pcAgeMs =
        pcMetrics_.freshness.available() ? millis() - pcMetrics_.freshness.lastUpdateMs() : 0;
    JsonObject pc = doc["pc"].to<JsonObject>();
    pc["available"] = pcMetrics_.freshness.available();
    pc["fresh"] = freshness.isFresh();
    pc["age_ms"] = pcAgeMs;
    pc["fetch_ok"] = pcMetricsService_.getFetchOkCount();
    pc["fetch_fail"] = pcMetricsService_.getFetchFailCount();
    pc["last_error"] = pcMetricsService_.getLastError();

    // pc_stream — SseConnection/PcMetricsStreamJob health, so the streaming
    // path can be observed without a serial monitor. State is meaningful even
    // when pcMetricsStreamEnabled is false: it just stays DISCONNECTED since
    // the job's nextDue() is always Never.
    JsonObject pcStream = doc["pc_stream"].to<JsonObject>();
    pcStream["enabled"] = config_.pcMetricsStreamEnabled;
    pcStream["state"] = sseStateToString(pcMetricsStreamJob_.connectionState());
    pcStream["reconnect_count"] = static_cast<unsigned long>(pcMetricsStreamJob_.reconnectCount());
    pcStream["last_event_age_ms"] = pcMetricsStreamJob_.lastEventAgeMs();
    pcStream["overflow_count"] = static_cast<unsigned long>(pcMetricsStreamJob_.overflowCount());

    // cpu_clock_stream / process_stream — same shape as pc_stream, for the
    // two opt-in, screen-gated streams. State is meaningful even when their
    // screen isn't active: it just stays DISCONNECTED since the job's
    // nextDue() is Never off-screen.
    JsonObject cpuClockStream = doc["cpu_clock_stream"].to<JsonObject>();
    cpuClockStream["state"] = sseStateToString(cpuClockStreamJob_.connectionState());
    cpuClockStream["reconnect_count"] =
        static_cast<unsigned long>(cpuClockStreamJob_.reconnectCount());
    cpuClockStream["last_event_age_ms"] = cpuClockStreamJob_.lastEventAgeMs();
    cpuClockStream["overflow_count"] =
        static_cast<unsigned long>(cpuClockStreamJob_.overflowCount());

    JsonObject processStream = doc["process_stream"].to<JsonObject>();
    processStream["state"] = sseStateToString(processStreamJob_.connectionState());
    processStream["reconnect_count"] =
        static_cast<unsigned long>(processStreamJob_.reconnectCount());
    processStream["last_event_age_ms"] = processStreamJob_.lastEventAgeMs();
    processStream["overflow_count"] =
        static_cast<unsigned long>(processStreamJob_.overflowCount());

    JsonObject ui = doc["ui"].to<JsonObject>();
    ui["screen"] = screenNameToString(systemState_.screen.activeScreen);
    ui["time_synced"] = systemState_.core.isTimeSynced;

    // weather — open-meteo forecast feed freshness, so the "fetch only while
    // displayed" gating can be observed without a serial monitor.
    // refresh_pending reflects WeatherWidget's midnight-rollover signal
    // awaiting the background job.
    const unsigned long weatherAgeMs =
        weatherData_.freshness.available() ? millis() - weatherData_.freshness.lastUpdateMs() : 0;
    JsonObject weather = doc["weather"].to<JsonObject>();
    weather["available"] = weatherData_.freshness.available();
    weather["age_ms"] = weatherAgeMs;
    weather["days"] = weatherData_.dayCount;
    weather["refresh_pending"] = weatherData_.refreshRequested.load();

    // room — local LAN temperature/humidity sensor feed, so the room widget's
    // data path can be observed without a serial monitor — same rationale as
    // pc_stream/audio above.
    const unsigned long roomAgeMs = roomClimateData_.freshness.available()
                                        ? millis() - roomClimateData_.freshness.lastUpdateMs()
                                        : 0;
    JsonObject room = doc["room"].to<JsonObject>();
    room["available"] = roomClimateData_.freshness.available();
    room["temperature_x10"] = roomClimateData_.temperature_x10;
    room["humidity"] = roomClimateData_.humidity;
    room["last_event_age_ms"] = roomAgeMs;

    // audio — mb_NerdBox MusicBee plugin push feed state, so the push path
    // (POST /audio) can be observed without a serial monitor or a visible
    // screen — see docs-local/NERDBOX_INTEGRATION.md.
    const unsigned long audioAgeMs =
        audioData_.freshness.available() ? millis() - audioData_.freshness.lastUpdateMs() : 0;
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
    audio["last_event_age_ms"] = audioAgeMs;
    // Age since the last stop/offline event, so "how long has it been
    // stopped" is observable without a screen — 0 while a track is loaded.
    audio["stopped_age_ms"] = audioData_.stopped ? millis() - audioData_.stoppedAtMs : 0;

    // Stack high-water marks — early warning before a tight task stack
    // (6144/8192 B) overflows and reboots the device. 0 means the task
    // hasn't been created yet.
    const TaskHandle_t screenTask = taskManager_.getScreenTaskHandle();
    const TaskHandle_t backgroundTask = taskManager_.getBackgroundTaskHandle();
    JsonObject tasks = doc["tasks"].to<JsonObject>();
    tasks["screen_stack_free"] = screenTask ? uxTaskGetStackHighWaterMark(screenTask) : 0;
    tasks["background_stack_free"] =
        backgroundTask ? uxTaskGetStackHighWaterMark(backgroundTask) : 0;

    ChunkedPrint output(server_);
    serializeJson(doc, output);
    output.flush();
    server_.sendContent("");  // flush / end chunked transfer
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
    {
        // disk_drives is written from core 0 — must hold the lock for the
        // duration of the read, same as any other disk_drives access.
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        for (size_t i = 0; i < pcMetrics_.disk_drives.size(); ++i) {
            const DiskDrive& drive = pcMetrics_.disk_drives[i];
            JsonObject d = drives.add<JsonObject>();
            d["name"] = drive.driveName;
            d["free_percent"] = roundToDecimals(drive.freeSpacePercent, 1);
            d["read_kbps"] = roundToDecimals(drive.readKBPerSec, 1);
            d["write_kbps"] = roundToDecimals(drive.writeKBPerSec, 1);
        }
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

    writeHeader(out, "nerdbox_pc_stream_reconnects_total", "SSE reconnect count since boot.",
                "counter");
    out.printf("nerdbox_pc_stream_reconnects_total %lu\n",
               static_cast<unsigned long>(pcMetricsStreamJob_.reconnectCount()));

    writeHeader(out, "nerdbox_pc_stream_overflow_total",
                "SSE event-buffer overflow count since boot.", "counter");
    out.printf("nerdbox_pc_stream_overflow_total %lu\n",
               static_cast<unsigned long>(pcMetricsStreamJob_.overflowCount()));

    writeHeader(out, "nerdbox_pc_stream_last_event_age_ms",
                "Milliseconds since the last SSE event was received.", "gauge");
    out.printf("nerdbox_pc_stream_last_event_age_ms %lu\n",
               static_cast<unsigned long>(pcMetricsStreamJob_.lastEventAgeMs()));

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

    // disk_drives is written from core 0 — snapshot under the lock, then
    // print after releasing it, same as the JSON-building pattern above but
    // with the copy landing in fixed-size locals instead of a JsonDocument.
    struct DiskSnapshot {
        char name[4];
        float freePercent;
        float readKBps;
        float writeKBps;
    };
    static constexpr size_t kMaxDisksForMetrics = 16;
    DiskSnapshot disks[kMaxDisksForMetrics];
    size_t diskCount = 0;
    {
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        diskCount = std::min(pcMetrics_.disk_drives.size(), kMaxDisksForMetrics);
        for (size_t i = 0; i < diskCount; ++i) {
            const DiskDrive& drive = pcMetrics_.disk_drives[i];
            strncpy(disks[i].name, drive.driveName, sizeof(disks[i].name) - 1);
            disks[i].name[sizeof(disks[i].name) - 1] = '\0';
            disks[i].freePercent = drive.freeSpacePercent;
            disks[i].readKBps = drive.readKBPerSec;
            disks[i].writeKBps = drive.writeKBPerSec;
        }
    }

    writeHeader(out, "nerdbox_pc_disk_free_percent", "Free space per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_free_percent{drive=\"%s\"} %.1f\n", disks[i].name,
                   disks[i].freePercent);
    }
    writeHeader(out, "nerdbox_pc_disk_read_kbps", "Read throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_read_kbps{drive=\"%s\"} %.1f\n", disks[i].name,
                   disks[i].readKBps);
    }
    writeHeader(out, "nerdbox_pc_disk_write_kbps", "Write throughput per drive.", "gauge");
    for (size_t i = 0; i < diskCount; ++i) {
        out.printf("nerdbox_pc_disk_write_kbps{drive=\"%s\"} %.1f\n", disks[i].name,
                   disks[i].writeKBps);
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

    out.flush();
    server_.sendContent("");  // flush / end chunked transfer
}
