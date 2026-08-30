#pragma once

#include <memory>
#include <vector>

#include "app/bundles/DataBundle.h"
#include "app/bundles/PlatformBundle.h"
#include "app/bundles/ServiceBundle.h"
#include "app/jobs/AirQualityJob.h"
#include "app/jobs/CpuClockStreamJob.h"
#include "app/jobs/DimAtNightJob.h"
#include "app/jobs/NetworkStatusJob.h"
#include "app/jobs/NtpRetryJob.h"
#include "app/jobs/PcMetricsJob.h"
#include "app/jobs/PcMetricsStreamJob.h"
#include "app/jobs/ProcessStreamJob.h"
#include "app/jobs/RoomClimateJob.h"
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
    RoomClimateJob roomClimateJob;
    NetworkStatusJob networkStatusJob;
    DimAtNightJob dimAtNightJob;
    WeatherJob weatherJob;

    // Heap-allocated rather than embedded inline: JobBundle is itself an
    // embedded member of ApplicationComponents, which is size-sensitive on
    // real hardware — growing its footprint (this job alone pulls in an
    // SseConnection with its own scratch buffers plus a JsonDocument) has
    // previously corrupted the LCD despite a clean build and passing host
    // tests (see [[applicationcomponents-size-sensitive]] in memory). A
    // separate heap allocation keeps ApplicationComponents' own block the
    // same size.
    std::unique_ptr<CpuClockStreamJob> cpuClockStreamJob;

    // Same heap-allocation rationale as cpuClockStreamJob — also carries an
    // SseConnection plus a JsonDocument.
    std::unique_ptr<ProcessStreamJob> processStreamJob;

    JobBundle(PlatformBundle& platform, DataBundle& data, ServiceBundle& services,
              const AppSettings& config)
        : wifiReconnectJob(platform.networkManager, data.systemState.core),
          ntpRetryJob(platform.ntpService, data.systemState.core, platform.logger_),
          pcMetricsJob(services.pcMetricsService, data.pcMetrics, data.systemState.core,
                       data.systemState.screen, platform.networkManager, config, platform.logger_),
          pcMetricsStreamJob(data.pcMetrics, data.systemState.core, data.systemState.screen,
                             platform.networkManager, config, platform.logger_,
                             services.systemMetrics),
          airQualityJob(services.airQualityService, data.airQualityData, data.systemState.core,
                        platform.networkManager, config, platform.logger_),
          roomClimateJob(services.roomClimateService, data.roomClimateData, data.systemState.core,
                         platform.networkManager, config, platform.logger_),
          networkStatusJob(services.networkStatusService, data.netStatus),
          dimAtNightJob(platform.ntpService, platform.displayManager, config),
          weatherJob(services.weatherService, data.weatherData, data.systemState.core,
                     platform.networkManager, config, platform.logger_),
          cpuClockStreamJob(std::make_unique<CpuClockStreamJob>(
              *data.cpuClockData, data.systemState.screen, platform.networkManager, config,
              platform.logger_)),
          processStreamJob(std::make_unique<ProcessStreamJob>(
              *data.processData, data.systemState.screen, platform.networkManager, config,
              platform.logger_)) {}

    std::vector<BackgroundJob*> asVector() {
        return {&wifiReconnectJob,       &ntpRetryJob,          &pcMetricsJob,
                &pcMetricsStreamJob,     &airQualityJob,        &roomClimateJob,
                &networkStatusJob,       &dimAtNightJob,        &weatherJob,
                cpuClockStreamJob.get(), processStreamJob.get()};
    }
};
