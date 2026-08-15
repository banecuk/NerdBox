#pragma once

#include <vector>

#include "config/AppSettings.h"
#include "core/bundles/DataBundle.h"
#include "core/bundles/PlatformBundle.h"
#include "core/bundles/ServiceBundle.h"
#include "core/BackgroundJob.h"
#include "core/jobs/AirQualityJob.h"
#include "core/jobs/DimAtNightJob.h"
#include "core/jobs/NetworkStatusJob.h"
#include "core/jobs/NtpRetryJob.h"
#include "core/jobs/PcMetricsJob.h"
#include "core/jobs/PcMetricsStreamJob.h"
#include "core/jobs/WeatherJob.h"
#include "core/jobs/WifiReconnectJob.h"

// One BackgroundJob adapter per periodic service, registered with
// TaskManager via asVector(). Adding a new periodic service means adding a
// job here and appending it to asVector()'s list.
struct JobBundle {
    WifiReconnectJob wifiReconnectJob;
    NtpRetryJob ntpRetryJob;
    PcMetricsJob pcMetricsJob;
    PcMetricsStreamJob pcMetricsStreamJob;
    AirQualityJob airQualityJob;
    NetworkStatusJob networkStatusJob;
    DimAtNightJob dimAtNightJob;
    WeatherJob weatherJob;

    JobBundle(PlatformBundle& platform, DataBundle& data, ServiceBundle& services,
              const AppSettings& config)
        : wifiReconnectJob(platform.networkManager, data.systemState.core),
          ntpRetryJob(platform.ntpService, data.systemState.core, platform.logger_),
          pcMetricsJob(services.pcMetricsService, data.pcMetrics, data.systemState.core,
                       data.systemState.screen, platform.networkManager, config,
                       platform.logger_),
          pcMetricsStreamJob(data.pcMetrics, data.systemState.core, data.systemState.screen,
                             platform.networkManager, config, platform.logger_),
          airQualityJob(services.airQualityService, data.airQualityData, data.systemState.core,
                        platform.networkManager, config, platform.logger_),
          networkStatusJob(services.networkStatusService, data.netStatus),
          dimAtNightJob(platform.ntpService, platform.displayManager, config),
          weatherJob(services.weatherService, data.weatherData, data.systemState.core,
                     platform.networkManager, config, platform.logger_) {}

    std::vector<BackgroundJob*> asVector() {
        return {&wifiReconnectJob,   &ntpRetryJob,       &pcMetricsJob,   &pcMetricsStreamJob,
                &airQualityJob,      &networkStatusJob,  &dimAtNightJob,  &weatherJob};
    }
};
