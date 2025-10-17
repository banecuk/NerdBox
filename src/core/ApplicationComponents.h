#pragma once

#include <WebServer.h>

#include <LovyanGFX.hpp>
#include <memory>

#include "config/AppConfig.h"
#include "config/AppConfigService.h"
#include "core/InitializationStateMachine.h"  // Add this include
#include "core/TaskManager.h"
#include "core/state/SystemState.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "ui/Colors.h"
#include "ui/DisplayManager.h"
#include "ui/UIController.h"
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
    UIController uiController;

    // Network Components
    HttpClient httpClient;
    NetworkManager networkManager;
    NtpService ntpService;

    // Services
    Logger logger;
    PcMetricsService pcMetricsService;
    HttpServer httpServer;
    WebServer webServer;

    // Managers
    ApplicationMetrics systemMetrics;
    TaskManager taskManager;
    InitializationStateMachine initStateMachine;

   private:
    void initializeComponents();
};