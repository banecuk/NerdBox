#include "ApplicationComponents.h"

ApplicationComponents::ApplicationComponents()
    : webServer(80),
      logger_(systemState.core.isTimeSynced),
      systemMetrics(config),
      displayContext(display, colors, logger_),
      networkManager(logger_, httpClient, config),
      displayManager(display, logger_),
      pcMetricsService(networkManager, systemMetrics, logger_, config),
      airQualityService(networkManager, logger_),
      networkStatusService(logger_),
      uiController(displayContext, &displayManager, systemMetrics, pcMetrics, systemState.screen,
                   config, networkManager, airQualityData, netStatus),
      webServerService(webServer, uiController, systemMetrics),
      taskManager(logger_, uiController, pcMetricsService, pcMetrics, airQualityService,
                  airQualityData, networkStatusService, netStatus, systemState.core,
                  systemState.screen, config, networkManager, ntpService),
      initStateMachine(*this) {}
