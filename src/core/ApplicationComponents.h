#pragma once

#include <WebServer.h>

#include <memory>

#include <LovyanGFX.hpp>

#include "config/AppSettings.h"
#include "core/IInitializationTarget.h"
#include "core/InitializationStateMachine.h"
#include "core/jobs/AirQualityJob.h"
#include "core/jobs/DimAtNightJob.h"
#include "core/jobs/NetworkStatusJob.h"
#include "core/jobs/NtpRetryJob.h"
#include "core/jobs/PcMetricsJob.h"
#include "core/jobs/PcMetricsStreamJob.h"
#include "core/jobs/WeatherJob.h"
#include "core/jobs/WifiReconnectJob.h"
#include "core/state/SystemState.h"
#include "core/TaskManager.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "services/network/NetworkStatus.h"
#include "services/network/NetworkStatusService.h"
#include "services/NtpService.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/weather/WeatherData.h"
#include "services/weather/WeatherService.h"
#include "services/WebServerService.h"
#include "ui/core/Colors.h"
#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

// ApplicationMetrics::kDrawTimesCapacity is redefined (not pulled from
// AppConfig) so utils/ stays independent of config/AppConfig.h — see that
// constant's comment. This is the one place both headers are already
// included, so it's where the two constants' agreement is enforced.
static_assert(ApplicationMetrics::kDrawTimesCapacity ==
                  AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes,
              "ApplicationMetrics::kDrawTimesCapacity and "
              "AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes must match");

class ApplicationComponents : public IInitializationTarget {
 public:
    ApplicationComponents();
    ~ApplicationComponents() = default;

    // Delete copy/move operations
    ApplicationComponents(const ApplicationComponents&) = delete;
    ApplicationComponents& operator=(const ApplicationComponents&) = delete;
    ApplicationComponents(ApplicationComponents&&) = delete;
    ApplicationComponents& operator=(ApplicationComponents&&) = delete;

    // -----------------------------------------------------------------------
    // IInitializationTarget implementation
    // -----------------------------------------------------------------------
    LoggerInterface& logger() override { return logger_; }

    void initializeDisplay() override { displayManager.initialize(); }
    void postInitializeDisplay() override { displayManager.postInitialization(); }

    void initializeUi() override { uiController.initialize(); }
    void requestScreen(ScreenName screen) override { uiController.requestScreen(screen); }

    bool createTasks() override { return taskManager.createTasks(); }

    bool connectNetwork() override { return networkManager.connect(); }
    bool isNetworkConnected() const override { return networkManager.isConnected(); }

    bool syncTime() override { return ntpService.syncTime(); }

    void beginWebServer() override { webServerService.begin(); }

    void setScreenInitialized() override { systemState.screen.isInitialized = true; }
    void setTimeSynced() override { systemState.core.isTimeSynced = true; }
    void setSystemInitialized() override { systemState.core.isInitialized = true; }

    uint8_t initTimeSyncRetries() const override { return config.initTimeSyncRetries; }
    uint32_t initTimeSyncBaseDelayMs() const override { return config.initTimeSyncBaseDelayMs; }
    uint16_t initBackoffJitterMs() const override { return config.initBackoffJitterMs; }
    bool watchdogEnabledOnBoot() const override { return config.watchdogEnableOnBoot; }
    unsigned long watchdogTimeoutMs() const override { return config.watchdogTimeoutMs; }

    // -----------------------------------------------------------------------
    // Public data — accessed by Application, TaskManager wiring, etc.
    //
    // Declaration order matters: members construct in declaration order
    // regardless of the constructor's initializer-list order, and several
    // members below are constructed from references to other members here.
    // This list is topologically sorted so every member is fully constructed
    // before anything that depends on it runs its own constructor — keep it
    // that way when adding new members (add near what it depends on, before
    // whatever will depend on it).
    // -----------------------------------------------------------------------

    // Core configuration and state — no dependencies.
    AppSettings config;
    SystemState systemState;

    // PC metrics data (standalone; not nested in SystemState to avoid
    // dragging the heavy PcMetrics type — which includes a std::vector and a
    // FreeRTOS semaphore — into every translation unit that needs SystemState).
    PcMetrics pcMetrics;

    // Air quality data — written by AirQualityService in the background task,
    // read by AirQualityWidget in the screen task. All fields are scalar so
    // no mutex is required (Xtensa word reads are atomic).
    AirQualityData airQualityData;

    // Network status — written by NetworkStatusService (background + probe tasks),
    // read by NetworkWidget (screen task). All scalar fields — no mutex needed.
    NetworkStatus netStatus;

    // Weather forecast data — written by WeatherService in the background task
    // only while the Weather screen is active, read by the (future) WeatherWidget
    // in the screen task. Scalars plus a fixed day array (no heap); only the
    // refreshRequested flag is shared cross-task and so is atomic.
    WeatherData weatherData;

    // Hardware — no dependencies.
    LGFX display;
    Colors colors;
    HttpClient httpClient;

    // Logger — depends only on systemState (bool& for isTimeSynced_).
    Logger logger_;  // named logger_ to avoid collision with IInitializationTarget::logger()

    // UI display plumbing — depends on display, colors, logger_.
    DisplayContext displayContext;
    DisplayManager displayManager;

    // Network — depends on logger_, httpClient, config.
    NetworkManager networkManager;
    NtpService ntpService;

    // Metrics aggregator — depends on config.
    ApplicationMetrics systemMetrics;

    // Services — depend on networkManager/logger_/config/systemMetrics above.
    PcMetricsService pcMetricsService;
    AirQualityService airQualityService;
    WeatherService weatherService;
    NetworkStatusService networkStatusService;

    // UI controller — depends on displayContext, displayManager, systemMetrics,
    // pcMetrics, systemState, config, networkManager, airQualityData, netStatus,
    // weatherData.
    UiController uiController;

    // Background jobs — one adapter per periodic service, registered with
    // TaskManager below. Adding a new periodic service means adding a job
    // here and appending it to the list passed to taskManager's constructor.
    WifiReconnectJob wifiReconnectJob;
    NtpRetryJob ntpRetryJob;
    PcMetricsJob pcMetricsJob;
    PcMetricsStreamJob pcMetricsStreamJob;
    AirQualityJob airQualityJob;
    NetworkStatusJob networkStatusJob;
    DimAtNightJob dimAtNightJob;
    WeatherJob weatherJob;

    // Managers — depend on everything above.
    TaskManager taskManager;

    // Web server — depends on uiController, systemMetrics, and taskManager
    // (stack high-water marks in /api/status), so it's declared after it.
    WebServer webServer;
    WebServerService webServerService;

    InitializationStateMachine initStateMachine;  // depends on *this; must stay last
};