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
#include "ui/core/UiController.h"
#include "utils/LoggerInterface.h"
#include "utils/ScopedLock.h"

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
    PcMetrics& pcMetrics_;
    PcMetricsService& pcMetricsService_;
    PcMetricsStreamJob& pcMetricsStreamJob_;
    const NetworkStatus& netStatus_;
    const SystemState& systemState_;
    const WeatherData& weatherData_;
    const AppSettings& config_;
    const TaskManager& taskManager_;
    LoggerInterface& logger_;

    void handleNotFound();
    void handleHome();
    void handleFavicon();
    void handleSystemInfo();
    void handleAppInfo();
    void handleApiStatus();
    void handleApiHelp();
    void handleApiRaw();
    void handleApiPc();
    void handleConfig();
    void handleLogs();
    void handleRestart();

    // Streams the HTML wrapper and body content directly to the client in
    // chunks — no large String assembled on the heap.
    void sendHtmlBegin(const char* title);
    void sendHtmlEnd();
    void sendSystemInfoBody();
    void sendAppInfoBody();

    static const char* internetStatusToString(NetworkStatus::Internet status);
    static const char* screenNameToString(ScreenName screen);
    static const char* sseStateToString(SseConnection::State state);
    static const char* logLevelToString(LoggerInterface::LogLevel level);
};