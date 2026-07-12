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
      wifiReconnectJob(networkManager, systemState.core),
      ntpRetryJob(ntpService, systemState.core, logger_),
      pcMetricsJob(pcMetricsService, pcMetrics, systemState.core, systemState.screen,
                   networkManager, config, logger_),
      pcMetricsStreamJob(pcMetrics, systemState.core, systemState.screen, networkManager, config,
                        logger_),
      airQualityJob(airQualityService, airQualityData, systemState.core, networkManager, config,
                    logger_),
      networkStatusJob(networkStatusService, netStatus),
      dimAtNightJob(ntpService, displayManager, config),
      taskManager(logger_, uiController, config, systemState.screen,
                  std::vector<BackgroundJob*>{&wifiReconnectJob, &ntpRetryJob, &pcMetricsJob,
                                              &pcMetricsStreamJob, &airQualityJob,
                                              &networkStatusJob, &dimAtNightJob}),
      webServer(80),
      webServerService(webServer, uiController, systemMetrics, pcMetrics, pcMetricsService,
                       pcMetricsStreamJob, netStatus, systemState, config, taskManager, logger_),
      initStateMachine(*this) {}
