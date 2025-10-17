#include "ApplicationComponents.h"

ApplicationComponents::ApplicationComponents()
    : webServer(80),
      logger(systemState.core.isTimeSynced),
      systemMetrics(config),
      displayContext(display, colors, logger),
      networkManager(logger, httpClient, config),
      displayManager(display, logger),
      pcMetricsService(networkManager, systemMetrics, logger, config),
      uiController(displayContext, &displayManager, systemMetrics, systemState.pcMetrics,
                   systemState.screen, config),
      httpServer(uiController, systemMetrics),
      taskManager(logger, uiController, pcMetricsService, systemState.pcMetrics,
                  systemState.core, systemState.screen, config),
      initStateMachine(*this, logger) {}