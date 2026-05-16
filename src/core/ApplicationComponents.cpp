#include "ApplicationComponents.h"

ApplicationComponents::ApplicationComponents()
    : webServer(80),
      logger(systemState.core.isTimeSynced),
      systemMetrics(config),
      displayContext(display, colors, logger),
      networkManager(logger, httpClient, config),
      displayManager(display, logger),
      pcMetricsService(networkManager, systemMetrics, logger, config),
      uiController(displayContext, &displayManager, systemMetrics, pcMetrics,
                   systemState.screen, config, networkManager),
      webServerService(webServer, uiController, systemMetrics),
      taskManager(logger, uiController, pcMetricsService, pcMetrics, systemState.core,
                  systemState.screen, config, networkManager),
      initStateMachine(*this) {}