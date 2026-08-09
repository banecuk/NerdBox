#include "WebApiHandlers.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <WiFi.h>

#include <cmath>

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
                               PcMetricsStreamJob& pcMetricsStreamJob,
                               const NetworkStatus& netStatus, const SystemState& systemState,
                               const WeatherData& weatherData, const AppSettings& config,
                               const TaskManager& taskManager)
    : server_(server),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      pcMetricsService_(pcMetricsService),
      pcMetricsStreamJob_(pcMetricsStreamJob),
      netStatus_(netStatus),
      systemState_(systemState),
      weatherData_(weatherData),
      config_(config),
      taskManager_(taskManager) {}

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
        case ScreenName::WEATHER:
            return "WEATHER";
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
    app["draw_avg_ms"] = roundToDecimals(systemMetrics_.getAverageScreenDrawTime(), 1);
    app["widget_fps"] = roundToDecimals(systemMetrics_.getThreadWidgetFPS(), 1);

    JsonArray drawTimes = app["draw_times"].to<JsonArray>();
    const auto& screenDrawTimes = systemMetrics_.getScreenDrawTimes();
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
    for (uint8_t i = 0; i < 6; ++i) {
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
    doc["cpu_temperature"] = pcMetrics_.cpu_temperature;
    doc["gpu_temperature"] = pcMetrics_.gpu_temperature;
    doc["cpu_load"] = pcMetrics_.cpu_load;
    doc["gpu_load"] = pcMetrics_.gpu_load;
    doc["mem_load"] = pcMetrics_.mem_load;
    doc["cpu_power"] = pcMetrics_.cpu_power;
    doc["gpu_power"] = pcMetrics_.gpu_power;
    doc["cpu_fan"] = pcMetrics_.cpu_fan;
    doc["gpu_fan"] = pcMetrics_.gpu_fan;

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

    doc["gpu_3d"] = pcMetrics_.gpu_3d;
    doc["gpu_compute"] = pcMetrics_.gpu_compute;
    doc["gpu_decode"] = pcMetrics_.gpu_decode;
    doc["gpu_mem"] = pcMetrics_.gpu_mem;
    doc["gpu_fps"] = pcMetrics_.gpu_fps;
    doc["eth_up"] = roundToDecimals(pcMetrics_.eth_up, 2);
    doc["eth_dn"] = roundToDecimals(pcMetrics_.eth_dn, 2);

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
