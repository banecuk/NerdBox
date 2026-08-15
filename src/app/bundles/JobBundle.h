#pragma once

#include <vector>

#include "app/bundles/DataBundle.h"
#include "app/bundles/PlatformBundle.h"
#include "app/bundles/ServiceBundle.h"
#include "app/jobs/AirQualityJob.h"
#include "app/jobs/DimAtNightJob.h"
#include "app/jobs/NetworkStatusJob.h"
#include "app/jobs/NtpRetryJob.h"
#include "app/jobs/PcMetricsJob.h"
#include "app/jobs/PcMetricsStreamJob.h"
#include "app/jobs/WeatherJob.h"
#include "app/jobs/WifiReconnectJob.h"
#include "config/AppSettings.h"
#include "core/BackgroundJob.h"

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
                       data.systemState.screen, platform.networkManager, config, platform.logger_),
          pcMetricsStreamJob(data.pcMetrics, data.systemState.core, data.systemState.screen,
                             platform.networkManager, config, platform.logger_),
          airQualityJob(services.airQualityService, data.airQualityData, data.systemState.core,
                        platform.networkManager, config, platform.logger_),
          networkStatusJob(services.networkStatusService, data.netStatus),
          dimAtNightJob(platform.ntpService, platform.displayManager, config),
          weatherJob(services.weatherService, data.weatherData, data.systemState.core,
                     platform.networkManager, config, platform.logger_) {}

    std::vector<BackgroundJob*> asVector() {
        return {&wifiReconnectJob, &ntpRetryJob,      &pcMetricsJob,  &pcMetricsStreamJob,
                &airQualityJob,    &networkStatusJob, &dimAtNightJob, &weatherJob};
    }
};
