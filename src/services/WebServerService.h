#pragma once

#include <WebServer.h>

#include "config/AppSettings.h"
#include "core/jobs/PcMetricsStreamJob.h"
#include "core/state/SystemState.h"
#include "core/TaskManager.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/weather/WeatherData.h"
#include "services/web/WebApiHandlers.h"
#include "ui/core/UiController.h"
#include "utils/LoggerInterface.h"

class WebServerService {
 public:
    WebServerService(WebServer& server, UiController& uiController,
                     ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                     PcMetricsService& pcMetricsService, PcMetricsStreamJob& pcMetricsStreamJob,
                     const NetworkStatus& netStatus, const SystemState& systemState,
                     const WeatherData& weatherData, const AppSettings& config,
                     const TaskManager& taskManager, LoggerInterface& logger);
    void begin();
    void processRequests();

 private:
    WebServer& server_;
    UiController& uiController_;
    ApplicationMetrics& systemMetrics_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    // The /api/* JSON handlers live in their own unit — see WebApiHandlers.h.
    // PcMetrics, PcMetricsService, PcMetricsStreamJob, NetworkStatus,
    // SystemState, WeatherData, and TaskManager are only needed there, so
    // WebServerService takes them as constructor parameters without keeping
    // its own members.
    WebApiHandlers apiHandlers_;

    void handleNotFound();
    void handleHome();
    void handleFavicon();
    void handleSystemInfo();
    void handleAppInfo();
    void handleApiHelp();
    void handleConfig();
    void handleLogs();
    void handleRestart();

    // Identifies which nav-bar link to highlight as active in sendHtmlBegin.
    // Mirrors the pages listed in WebAssets::kHtmlHead2/kHtmlHead3's nav.
    enum class NavPage { kHome, kAppInfo, kSystemInfo, kLogs, kConfig, kApi };

    // Streams the HTML wrapper and body content directly to the client in
    // chunks — no large String assembled on the heap.
    void sendHtmlBegin(const char* title, NavPage activePage);
    void sendNavLink(const char* href, const char* label, bool active);
    void sendHtmlEnd();
    void sendSystemInfoBody();
    void sendAppInfoBody();

    static const char* logLevelToString(LoggerInterface::LogLevel level);
};