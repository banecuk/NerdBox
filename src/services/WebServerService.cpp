#include "WebServerService.h"

#include "core/events/EventBus.h"
#include "services/web/WebAssets.h"

WebServerService::WebServerService(WebServer& server, UiController& uiController,
                                   ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                                   PcMetricsService& pcMetricsService,
                                   PcMetricsStreamJob& pcMetricsStreamJob,
                                   const NetworkStatus& netStatus, const SystemState& systemState,
                                   const WeatherData& weatherData, const AppSettings& config,
                                   const TaskManager& taskManager, LoggerInterface& logger)
    : server_(server),
      uiController_(uiController),
      systemMetrics_(systemMetrics),
      config_(config),
      logger_(logger),
      apiHandlers_(server, systemMetrics, pcMetrics, pcMetricsService, pcMetricsStreamJob,
                   netStatus, systemState, weatherData, config, taskManager) {}

void WebServerService::begin() {
    server_.on("/", [this]() { this->handleHome(); });
    server_.on("/system-info", [this]() { this->handleSystemInfo(); });
    server_.on("/app-info", [this]() { this->handleAppInfo(); });
    server_.on("/api", [this]() { this->handleApiHelp(); });
    server_.on("/api/status", [this]() { apiHandlers_.handleApiStatus(); });
    server_.on("/api/raw", [this]() { apiHandlers_.handleApiRaw(); });
    server_.on("/api/pc", [this]() { apiHandlers_.handleApiPc(); });
    server_.on("/metrics", [this]() { apiHandlers_.handleMetrics(); });
    server_.on("/config", [this]() { this->handleConfig(); });
    server_.on("/logs", [this]() { this->handleLogs(); });
    server_.on("/restart", HTTP_POST, [this]() { this->handleRestart(); });
    server_.on("/favicon.ico", [this]() { this->handleFavicon(); });
    // POST-only: these change device state, so a GET (e.g. a LAN prefetcher
    // or a link crawler) can no longer flip the screen as a side effect.
    server_.on("/screen/main", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::MAIN);
        server_.send(200, "text/plain", "OK");
    });
    server_.on("/screen/settings", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::SETTINGS);
        server_.send(200, "text/plain", "OK");
    });
    server_.on("/screen/game", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::GAME);
        server_.send(200, "text/plain", "OK");
    });
    server_.on("/screen/weather", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::WEATHER);
        server_.send(200, "text/plain", "OK");
    });
    server_.onNotFound([this]() { this->handleNotFound(); });
    server_.begin();
}

void WebServerService::processRequests() {
    server_.handleClient();
}

void WebServerService::handleNotFound() {
    server_.send(404, "text/plain", "Not found");
}

void WebServerService::handleFavicon() {
    // No icon shipped — respond empty instead of letting every browser visit
    // burn a 404.
    server_.send(204);
}

// ---------------------------------------------------------------------------
// Chunked streaming helpers
// ---------------------------------------------------------------------------

void WebServerService::sendHtmlBegin(const char* title) {
    // Tell the client we will stream the body — no Content-Length needed.
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");  // open the response
    server_.sendContent(WebAssets::kHtmlHead1);
    server_.sendContent(title);
    server_.sendContent(WebAssets::kHtmlHead2);
    server_.sendContent(title);  // repeated in the header's meta area
    server_.sendContent(WebAssets::kHtmlHead3);
}

void WebServerService::sendHtmlEnd() {
    server_.sendContent(WebAssets::kHtmlFoot);
    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Dashboard — single dark-themed page served whole from flash (send via
// chunked sendContent, zero per-request heap for the shell itself). All
// dynamic values are pulled client-side from GET /api/status on a 2 s timer,
// so this literal never needs to change per request. Markup lives in
// WebAssets.h alongside the other page-shell blobs.
// ---------------------------------------------------------------------------

void WebServerService::handleHome() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-cache");
    server_.send(200, "text/html", "");
    server_.sendContent(WebAssets::kDashboardHtml);
    server_.sendContent("");  // flush / end chunked transfer
}

void WebServerService::sendSystemInfoBody() {
    char buf[128];
    server_.sendContent("<pre>");
    snprintf(buf, sizeof(buf), "CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "PSRAM Size: %u bytes\n", ESP.getPsramSize());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "PSRAM Free: %u bytes\n", ESP.getFreePsram());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "SDK Version: %s\n", ESP.getSdkVersion());
    server_.sendContent(buf);
    server_.sendContent("</pre>");
}

void WebServerService::handleSystemInfo() {
    sendHtmlBegin("System Information");
    sendSystemInfoBody();
    sendHtmlEnd();
}

void WebServerService::sendAppInfoBody() {
    char buf[128];

    server_.sendContent("<pre>");

    char uptime[20];
    systemMetrics_.getFormattedUptime(uptime, sizeof(uptime));
    snprintf(buf, sizeof(buf), "Uptime: %s\n", uptime);
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Free Heap: %u bytes\n", ESP.getFreeHeap());
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "NerdWinSense JSON Parse Time: %u ms\n",
             systemMetrics_.getPcMetricsJsonParseTime());
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Average Screen Draw Time: %u ms\n",
             static_cast<uint32_t>(systemMetrics_.getAverageScreenDrawTime()));
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Thread Widget FPS: %.1f\n", systemMetrics_.getThreadWidgetFPS());
    server_.sendContent(buf);

    server_.sendContent("</pre>");

    // Draw-times table — streamed row by row, no giant String.
    server_.sendContent(
        "<table class='draw-times'>"
        "<tr><th>Draw</th><th>Draw time (ms)</th></tr>");

    const auto& drawTimes = systemMetrics_.getScreenDrawTimes();
    const size_t count = systemMetrics_.getScreenDrawCount();
    const size_t start = systemMetrics_.getScreenDrawStartIndex();

    // Walk the circular buffer starting at the oldest sample so "Draw N" is
    // chronological, not raw slot order.
    for (size_t i = 0; i < count && i < drawTimes.size(); ++i) {
        snprintf(buf, sizeof(buf), "<tr><td>%u</td><td>%u</td></tr>",
                 static_cast<unsigned int>(i + 1), drawTimes[(start + i) % drawTimes.size()]);
        server_.sendContent(buf);
    }

    server_.sendContent("</table>");
}

void WebServerService::handleAppInfo() {
    sendHtmlBegin("App Information");
    sendAppInfoBody();
    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /restart — remote recovery without a power cycle.
// ---------------------------------------------------------------------------

void WebServerService::handleRestart() {
    logger_.info("Restart requested via web server");
    server_.send(200, "text/plain", "Restarting...");
    server_.client().flush();
    delay(200);  // give the TCP stack time to push the response before reboot
    EventBus::getInstance().publish(EventType::RESET_DEVICE);
}

// ---------------------------------------------------------------------------
// /api — help page listing every API endpoint this web server provides.
// ---------------------------------------------------------------------------

namespace {
struct ApiEndpoint {
    const char* method;
    const char* path;
    const char* description;
};

constexpr ApiEndpoint kApiEndpoints[] = {
    {"GET",  "/api/status",
     "Compact JSON snapshot: system health, app metrics, network, PC-metrics feed "
     "status, current screen, task stack high-water marks. Cache-Control: no-store."   },
    {"GET",  "/api/raw",
     "Last raw NerdWinSense payload, unfiltered — for debugging a field that reads "
     "0 or -1. Triggers a dedicated fetch; not the same connection as the regular "
     "poll."                                                                           },
    {"GET",  "/api/pc",
     "Full current PcMetrics snapshot as JSON — every parsed field, including disk "
     "drives. Verify what the display should show vs. what NerdWinSense sent."         },
    {"GET",  "/metrics",
     "Prometheus text-exposition of the same data as /api/status and /api/pc — scrape "
     "target for Prometheus/Grafana, or a bonus sensor source for Home Assistant."     },
    {"POST", "/restart",         "Reboots the device (ESP.restart()). No confirmation."},
    {"POST", "/screen/main",     "Switches the display to the Main screen."            },
    {"POST", "/screen/settings", "Switches the display to the Settings screen."        },
    {"POST", "/screen/game",     "Switches the display to the Game screen."            },
    {"POST", "/screen/weather",  "Switches the display to the Weather screen."         },
};
}  // namespace

void WebServerService::handleApiHelp() {
    sendHtmlBegin("API");

    server_.sendContent(
        "<p>Endpoints provided by this web server. JSON endpoints are safe to poll "
        "from scripts (curl, Home Assistant, etc.); POST endpoints change device state.</p>"
        "<table><tr><th>Method</th><th>Path</th><th>Description</th></tr>");

    char row[384];
    for (const auto& endpoint : kApiEndpoints) {
        // Only GET endpoints are linked — a POST route (/restart, /screen/*)
        // would just 404 on a plain click since it can't send the method.
        char pathCell[192];
        if (strcmp(endpoint.method, "GET") == 0) {
            snprintf(pathCell, sizeof(pathCell), "<a href='%s' target='_blank'><code>%s</code></a>",
                     endpoint.path, endpoint.path);
        } else {
            snprintf(pathCell, sizeof(pathCell), "<code>%s</code>", endpoint.path);
        }
        snprintf(row, sizeof(row), "<tr><td>%s</td><td>%s</td><td>%s</td></tr>", endpoint.method,
                 pathCell, endpoint.description);
        server_.sendContent(row);
    }
    server_.sendContent("</table>");

    server_.sendContent(
        "<p>Human-oriented pages: <code>/</code>, <code>/system-info</code>, "
        "<code>/app-info</code>, <code>/config</code> (active tuning constants), "
        "<code>/logs</code> (recent log entries).</p>");

    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /config — active AppSettings values, so a flashed device can be confirmed
// to be running the tuning constants (and debug/release build) expected.
// ---------------------------------------------------------------------------

void WebServerService::handleConfig() {
    sendHtmlBegin("Config");
    char buf[128];

    server_.sendContent("<pre>");

#define SEND_CONFIG_U(name, value)                                                          \
    do {                                                                                    \
        snprintf(buf, sizeof(buf), "%-32s %lu\n", name, static_cast<unsigned long>(value)); \
        server_.sendContent(buf);                                                           \
    } while (0)
#define SEND_CONFIG_F(name, value)                                 \
    do {                                                           \
        snprintf(buf, sizeof(buf), "%-32s %.3f\n", name, (value)); \
        server_.sendContent(buf);                                  \
    } while (0)
#define SEND_CONFIG_B(name, value)                                                  \
    do {                                                                            \
        snprintf(buf, sizeof(buf), "%-32s %s\n", name, (value) ? "true" : "false"); \
        server_.sendContent(buf);                                                   \
    } while (0)

    SEND_CONFIG_U("debugSerialBaudRate", config_.debugSerialBaudRate);
    SEND_CONFIG_U("debugSerialTimeoutMs", config_.debugSerialTimeoutMs);
    SEND_CONFIG_B("debugWaitForSerial", config_.debugWaitForSerial);
    SEND_CONFIG_U("initNetworkRetries", config_.initNetworkRetries);
    SEND_CONFIG_U("initNetworkRetryDelayMs", config_.initNetworkRetryDelayMs);
    SEND_CONFIG_U("initTimeSyncRetries", config_.initTimeSyncRetries);
    SEND_CONFIG_U("initTimeSyncBaseDelayMs", config_.initTimeSyncBaseDelayMs);
    SEND_CONFIG_U("initBackoffJitterMs", config_.initBackoffJitterMs);
    SEND_CONFIG_U("watchdogTimeoutMs", config_.watchdogTimeoutMs);
    SEND_CONFIG_B("watchdogEnableOnBoot", config_.watchdogEnableOnBoot);
    SEND_CONFIG_U("timingScreenTaskMs", config_.timingScreenTaskMs);
    SEND_CONFIG_U("timingBackgroundTaskMs", config_.timingBackgroundTaskMs);
    SEND_CONFIG_U("timingMainLoopMs", config_.timingMainLoopMs);
    SEND_CONFIG_U("tasksScreenStack", config_.tasksScreenStack);
    SEND_CONFIG_U("tasksBackgroundStack", config_.tasksBackgroundStack);
    SEND_CONFIG_U("tasksScreenPriority", config_.tasksScreenPriority);
    SEND_CONFIG_U("tasksBackgroundPriority", config_.tasksBackgroundPriority);
    SEND_CONFIG_U("hardwareMonitorRefreshMs", config_.hardwareMonitorRefreshMs);
    SEND_CONFIG_U("hardwareMonitorThreadsRefreshMs", config_.hardwareMonitorThreadsRefreshMs);
    SEND_CONFIG_U("hardwareMonitorFailureRefreshMs", config_.hardwareMonitorFailureRefreshMs);
    SEND_CONFIG_U("hardwareMonitorRetryDelayMs", config_.hardwareMonitorRetryDelayMs);
    SEND_CONFIG_U("hardwareMonitorMaxRetries", config_.hardwareMonitorMaxRetries);
    SEND_CONFIG_F("hardwareMonitorThreadsUpwardSmoothing",
                  config_.hardwareMonitorThreadsUpwardSmoothing);
    SEND_CONFIG_F("hardwareMonitorThreadsDownwardSmoothing",
                  config_.hardwareMonitorThreadsDownwardSmoothing);
    SEND_CONFIG_U("airQualityFailureBackoffMs", config_.airQualityFailureBackoffMs);
    SEND_CONFIG_U("weatherRefreshIntervalMs", config_.weatherRefreshIntervalMs);
    SEND_CONFIG_U("weatherTimeCheckIntervalMs", config_.weatherTimeCheckIntervalMs);
    SEND_CONFIG_U("weatherFailureBackoffMs", config_.weatherFailureBackoffMs);
    SEND_CONFIG_U("weatherForecastDays", config_.weatherForecastDays);
    SEND_CONFIG_U("metricsMaxScreenDrawTimes", config_.metricsMaxScreenDrawTimes);
    SEND_CONFIG_U("pcMetricsCores", config_.pcMetricsCores);
    SEND_CONFIG_B("pcMetricsStreamEnabled", config_.pcMetricsStreamEnabled);
    SEND_CONFIG_U("pcMetricsStreamIntervalMs", config_.pcMetricsStreamIntervalMs);
    SEND_CONFIG_B("pcMetricsStreamDelta", config_.pcMetricsStreamDelta);
    SEND_CONFIG_U("pcMetricsStreamConnectTimeoutMs", config_.pcMetricsStreamConnectTimeoutMs);
    SEND_CONFIG_U("pcMetricsStreamHeaderTimeoutMs", config_.pcMetricsStreamHeaderTimeoutMs);
    SEND_CONFIG_U("pcMetricsStreamReconnectBackoffMs", config_.pcMetricsStreamReconnectBackoffMs);
    SEND_CONFIG_U("pcMetricsStreamMaxEventBufferBytes", config_.pcMetricsStreamMaxEventBufferBytes);
    SEND_CONFIG_U("pcMetricsStreamMaxBytesPerPoll", config_.pcMetricsStreamMaxBytesPerPoll);
    SEND_CONFIG_U("uiTransitionTimeoutMs", config_.uiTransitionTimeoutMs);
    SEND_CONFIG_U("uiTouchDebounceIntervalMs", config_.uiTouchDebounceIntervalMs);
    SEND_CONFIG_U("uiScreenTransitionCooldownMs", config_.uiScreenTransitionCooldownMs);
    SEND_CONFIG_U("uiDisplayLockTimeoutMs", config_.uiDisplayLockTimeoutMs);

#undef SEND_CONFIG_U
#undef SEND_CONFIG_F
#undef SEND_CONFIG_B

    snprintf(buf, sizeof(buf), "%-32s %s\n", "buildMode",
#if DEBUG_MODE
             "debug"
#else
             "release"
#endif
    );
    server_.sendContent(buf);

    server_.sendContent("</pre>");
    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /logs — recent log entries (level >= INFO), non-destructive.
// ---------------------------------------------------------------------------

const char* WebServerService::logLevelToString(LoggerInterface::LogLevel level) {
    switch (level) {
        case LoggerInterface::LogLevel::DEBUG:
            return "DEBUG";
        case LoggerInterface::LogLevel::INFO:
            return "INFO";
        case LoggerInterface::LogLevel::WARNING:
            return "WARNING";
        case LoggerInterface::LogLevel::ERROR:
            return "ERROR";
        case LoggerInterface::LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

void WebServerService::handleLogs() {
    sendHtmlBegin("Logs");

    // Heap-allocated — kRecentLogCapacity * sizeof(LogEntry) is too big for a
    // comfortable stack frame on the Arduino loop task.
    auto entries =
        std::make_unique<LoggerInterface::LogEntry[]>(LoggerInterface::kRecentLogCapacity);
    const size_t count = logger_.copyRecentLogs(entries.get(), LoggerInterface::kRecentLogCapacity);

    char row[256];
    if (count == 0) {
        server_.sendContent("<p>No log entries recorded yet.</p>");
    } else {
        server_.sendContent("<table><tr><th>Time</th><th>Level</th><th>Message</th></tr>");
        for (size_t i = 0; i < count; ++i) {
            const LoggerInterface::LogEntry& entry = entries[i];
            snprintf(row, sizeof(row), "<tr><td>%s</td><td>%s</td><td>%s</td></tr>",
                     entry.timestamp, logLevelToString(entry.level), entry.message);
            server_.sendContent(row);
        }
        server_.sendContent("</table>");
    }

    sendHtmlEnd();
}
