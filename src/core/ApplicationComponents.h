#pragma once

#include <WebServer.h>

#include <memory>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "config/AppConfigService.h"
#include "core/InitializationStateMachine.h"
#include "core/state/SystemState.h"
#include "core/TaskManager.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/NtpService.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/WebServerService.h"
#include "ui/core/Colors.h"
#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

class ApplicationComponents {
 public:
    ApplicationComponents();
    ~ApplicationComponents() = default;

    // Delete copy/move operations
    ApplicationComponents(const ApplicationComponents&) = delete;
    ApplicationComponents& operator=(const ApplicationComponents&) = delete;
    ApplicationComponents(ApplicationComponents&&) = delete;
    ApplicationComponents& operator=(ApplicationComponents&&) = delete;

    // Core configuration and state
    AppConfigService config;
    SystemState systemState;

    // Hardware
    LGFX display;

    // UI Components
    Colors colors;
    DisplayContext displayContext;
    DisplayManager displayManager;
    UiController uiController;

    // Network Components
    HttpClient httpClient;
    NetworkManager networkManager;
    NtpService ntpService;

    // Services
    Logger logger;
    PcMetricsService pcMetricsService;
    WebServerService webServerService;
    WebServer webServer;

    // Managers
    ApplicationMetrics systemMetrics;
    TaskManager taskManager;
    InitializationStateMachine initStateMachine;

 private:
    void initializeComponents();
};