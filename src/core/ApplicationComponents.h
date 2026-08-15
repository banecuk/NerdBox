#pragma once

#include <WebServer.h>

#include "config/AppSettings.h"
#include "core/bundles/DataBundle.h"
#include "core/bundles/JobBundle.h"
#include "core/bundles/PlatformBundle.h"
#include "core/bundles/ServiceBundle.h"
#include "core/IInitializationTarget.h"
#include "core/InitializationStateMachine.h"
#include "core/TaskManager.h"
#include "services/WebServerService.h"
#include "ui/core/UiController.h"

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
    // IInitializationTarget implementation — see ApplicationComponents.cpp
    // -----------------------------------------------------------------------
    LoggerInterface& logger() override;

    void initializeDisplay() override;
    void postInitializeDisplay() override;

    void initializeUi() override;
    void requestScreen(ScreenName screen) override;

    bool createTasks() override;

    bool connectNetwork() override;
    bool isNetworkConnected() const override;

    bool syncTime() override;

    void beginWebServer() override;

    void setScreenInitialized() override;
    void setTimeSynced() override;
    void setSystemInitialized() override;

    uint8_t initTimeSyncRetries() const override;
    uint32_t initTimeSyncBaseDelayMs() const override;
    uint16_t initBackoffJitterMs() const override;
    bool watchdogEnabledOnBoot() const override;
    unsigned long watchdogTimeoutMs() const override;

    // -----------------------------------------------------------------------
    // Public data — accessed by Application, TaskManager wiring, etc.
    //
    // Declaration order matters: members construct in declaration order
    // regardless of the constructor's initializer-list order. Each bundle
    // resolves its own internal ordering constraints (see the comment in
    // each bundle header); across bundles, later ones depend on earlier ones
    // — keep that order when adding a new bundle or trio member.
    // -----------------------------------------------------------------------

    AppSettings config;

    DataBundle data;
    PlatformBundle platform;
    ServiceBundle services;
    JobBundle jobs;

    // UI controller — depends on platform (displayContext/displayManager/
    // networkManager), services (systemMetrics), data (pcMetrics,
    // systemState, airQualityData, netStatus, weatherData), and config.
    UiController uiController;

    // Managers — depend on everything above.
    TaskManager taskManager;

    // Web server — depends on uiController, services, taskManager (stack
    // high-water marks in /api/status), so it's declared after it.
    WebServer webServer;
    WebServerService webServerService;

    InitializationStateMachine initStateMachine;  // depends on *this; must stay last
};
