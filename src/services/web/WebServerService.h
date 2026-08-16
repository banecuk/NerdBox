#pragma once

#include <WebServer.h>

#include "app/jobs/PcMetricsStreamJob.h"
#include "app/TaskManager.h"
#include "config/AppSettings.h"
#include "core/IScreenNavigator.h"
#include "core/state/SystemState.h"
#include "services/audio/AudioData.h"
#include "services/audio/AudioService.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/weather/WeatherData.h"
#include "services/web/WebApiHandlers.h"
#include "services/web/WebPageHandlers.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/RecentLogView.h"

// Routing only: registers every route with the underlying WebServer and
// dispatches to WebApiHandlers (JSON/Prometheus endpoints) or
// WebPageHandlers (human-facing HTML pages). /restart is the one handler
// that stays here — it's neither JSON nor a rendered page, just an
// EventBus publish.
class WebServerService {
 public:
    WebServerService(WebServer& server, IScreenNavigator& screenNavigator,
                     ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                     PcMetricsService& pcMetricsService, PcMetricsStreamJob& pcMetricsStreamJob,
                     const NetworkStatus& netStatus, const SystemState& systemState,
                     const WeatherData& weatherData, const AppSettings& config,
                     const TaskManager& taskManager, LoggerInterface& logger,
                     RecentLogView& recentLogView, const AudioData& audioData,
                     AudioService& audioService);
    void begin();
    void processRequests();

 private:
    WebServer& server_;
    IScreenNavigator& screenNavigator_;
    LoggerInterface& logger_;
    AudioService& audioService_;

    // PcMetrics, PcMetricsService, PcMetricsStreamJob, NetworkStatus,
    // SystemState, WeatherData, and TaskManager are only needed by
    // apiHandlers_; ApplicationMetrics/AppSettings/RecentLogView only by
    // pageHandlers_ — so WebServerService takes them as constructor
    // parameters without keeping its own members.
    WebApiHandlers apiHandlers_;
    WebPageHandlers pageHandlers_;

    void handleRestart();
    void handleAudioPush();
};