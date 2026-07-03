#pragma once

#include <WebServer.h>

#include <memory>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "config/AppConfigService.h"
#include "core/IInitializationTarget.h"
#include "core/InitializationStateMachine.h"
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
#include "services/WebServerService.h"
#include "ui/core/Colors.h"
#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

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

    uint8_t initTimeSyncRetries() const override { return config.getInitTimeSyncRetries(); }
    uint32_t initTimeSyncBaseDelayMs() const override {
        return config.getInitTimeSyncBaseDelayMs();
    }
    uint16_t initBackoffJitterMs() const override { return config.getInitBackoffJitterMs(); }
    bool watchdogEnabledOnBoot() const override { return config.getWatchdogEnableOnBoot(); }
    unsigned long watchdogTimeoutMs() const override { return config.getWatchdogTimeoutMs(); }

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
    AppConfigService config;
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
    NetworkStatusService networkStatusService;

    // UI controller — depends on displayContext, displayManager, systemMetrics,
    // pcMetrics, systemState, config, networkManager, airQualityData, netStatus.
    UiController uiController;

    // Web server — depends on uiController, systemMetrics.
    WebServer webServer;
    WebServerService webServerService;

    // Managers — depend on everything above.
    TaskManager taskManager;
    InitializationStateMachine initStateMachine;  // depends on *this; must stay last
};