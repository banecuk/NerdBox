#include "ApplicationComponents.h"

// Initializer list is written in the same order as the member declarations
// in ApplicationComponents.h (which is what actually governs construction
// order) so the two stay easy to keep in sync — see the comment there.
ApplicationComponents::ApplicationComponents()
    : logger_(systemState.core.isTimeSynced),
      displayContext(display, colors, logger_),
      displayManager(display, logger_),
      networkManager(logger_, httpClient, config),
      pcMetricsService(networkManager, systemMetrics, logger_, config),
      airQualityService(networkManager, logger_),
      networkStatusService(logger_),
      uiController(displayContext, &displayManager, systemMetrics, pcMetrics, systemState.screen,
                   config, networkManager, airQualityData, netStatus),
      webServer(80),
      webServerService(webServer, uiController, systemMetrics),
      taskManager(logger_, uiController, pcMetricsService, pcMetrics, airQualityService,
                  airQualityData, networkStatusService, netStatus, systemState.core,
                  systemState.screen, config, networkManager, ntpService),
      initStateMachine(*this) {}
