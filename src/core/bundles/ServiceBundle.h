#pragma once

#include "config/AppSettings.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityService.h"
#include "services/network/NetworkStatusService.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/weather/WeatherService.h"
#include "utils/ApplicationMetrics.h"
#include "utils/LoggerInterface.h"

// ApplicationMetrics::kDrawTimesCapacity is redefined (not pulled from
// AppConfig) so utils/ stays independent of config/AppConfig.h — see that
// constant's comment. This is the one place both headers are already
// included, so it's where the two constants' agreement is enforced.
static_assert(ApplicationMetrics::kDrawTimesCapacity ==
                  AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes,
              "ApplicationMetrics::kDrawTimesCapacity and "
              "AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes must match");

// Services that fetch/aggregate data over the network, plus the metrics
// aggregator they report through. Declaration order: systemMetrics has no
// dependencies; every service below depends on networkManager/logger_/config
// (and pcMetricsService additionally on systemMetrics).
struct ServiceBundle {
    ApplicationMetrics systemMetrics;

    PcMetricsService pcMetricsService;
    AirQualityService airQualityService;
    WeatherService weatherService;
    NetworkStatusService networkStatusService;

    ServiceBundle(NetworkManager& networkManager, LoggerInterface& logger,
                  const AppSettings& config)
        : pcMetricsService(networkManager, systemMetrics, logger, config),
          airQualityService(networkManager, logger),
          weatherService(networkManager, logger),
          networkStatusService(logger) {}
};
