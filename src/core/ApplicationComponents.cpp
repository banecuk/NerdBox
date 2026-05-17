#include "ApplicationComponents.h"

ApplicationComponents::ApplicationComponents()
    : webServer(80),
      logger(systemState.core.isTimeSynced),
      systemMetrics(config),
      displayContext(display, colors, logger),
      networkManager(logger, httpClient, config),
      displayManager(display, logger),
      pcMetricsService(networkManager, systemMetrics, logger, config),
      airQualityService(networkManager, logger),
      networkStatusService(logger),
      uiController(displayContext, &displayManager, systemMetrics, pcMetrics,
                   systemState.screen, config, networkManager, airQualityData, netStatus),
      webServerService(webServer, uiController, systemMetrics),
      taskManager(logger, uiController, pcMetricsService, pcMetrics,
                  airQualityService, airQualityData,
                  networkStatusService, netStatus,
                  systemState.core, systemState.screen, config, networkManager),
      initStateMachine(*this) {}