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
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/WebServerService.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "services/network/NetworkStatus.h"
#include "services/network/NetworkStatusService.h"
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
    AirQualityService airQualityService;
    NetworkStatusService networkStatusService;
    WebServerService webServerService;
    WebServer webServer;

    // Managers
    ApplicationMetrics systemMetrics;
    TaskManager taskManager;
    InitializationStateMachine initStateMachine;

 private:
    void initializeComponents();
};