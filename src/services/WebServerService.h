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
#include "services/web/WebPageHandlers.h"
#include "ui/core/UiController.h"
#include "utils/LoggerInterface.h"
#include "utils/RecentLogView.h"

// Routing only: registers every route with the underlying WebServer and
// dispatches to WebApiHandlers (JSON/Prometheus endpoints) or
// WebPageHandlers (human-facing HTML pages). /restart is the one handler
// that stays here — it's neither JSON nor a rendered page, just an
// EventBus publish.
class WebServerService {
 public:
    WebServerService(WebServer& server, UiController& uiController,
                     ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                     PcMetricsService& pcMetricsService, PcMetricsStreamJob& pcMetricsStreamJob,
                     const NetworkStatus& netStatus, const SystemState& systemState,
                     const WeatherData& weatherData, const AppSettings& config,
                     const TaskManager& taskManager, LoggerInterface& logger,
                     RecentLogView& recentLogView);
    void begin();
    void processRequests();

 private:
    WebServer& server_;
    UiController& uiController_;
    LoggerInterface& logger_;

    // PcMetrics, PcMetricsService, PcMetricsStreamJob, NetworkStatus,
    // SystemState, WeatherData, and TaskManager are only needed by
    // apiHandlers_; ApplicationMetrics/AppSettings/RecentLogView only by
    // pageHandlers_ — so WebServerService takes them as constructor
    // parameters without keeping its own members.
    WebApiHandlers apiHandlers_;
    WebPageHandlers pageHandlers_;

    void handleRestart();
};