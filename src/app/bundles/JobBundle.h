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
//
// SIZE-SENSITIVE: this bundle is embedded inline in ApplicationComponents —
// see the warning at the top of app/ApplicationComponents.h before adding or
// growing a member. This is the bundle that's hit it the most (twice: the
// CPU-clock/process stream jobs, then pcMetricsStreamJob below) — any new
// job that owns a nontrivial buffer (SseConnection, JsonDocument, a fixed
// array of more than a few dozen bytes) should default to
// std::unique_ptr<Job> from the start rather than an inline member.
struct JobBundle {
    WifiReconnectJob wifiReconnectJob;
    NtpRetryJob ntpRetryJob;
    PcMetricsJob pcMetricsJob;
    AirQualityJob airQualityJob;
    RoomClimateJob roomClimateJob;
    NetworkStatusJob networkStatusJob;
    DimAtNightJob dimAtNightJob;
    WeatherJob weatherJob;

    // Heap-allocated rather than embedded inline: JobBundle is itself an
    // embedded member of ApplicationComponents, which is size-sensitive on
    // real hardware — growing its footprint (each of these carries an
    // SseConnection with its own scratch buffers, plus a JsonDocument or
    // similar) has previously corrupted the LCD despite a clean build and
    // passing host tests (see [[applicationcomponents-size-sensitive]] in
    // memory). Separate heap allocations keep ApplicationComponents' own
    // block the same size regardless of how these three grow — all three now
    // derive from the shared SseStreamJob base (docs-local/11-code-quality.md
    // Q1), which added a path_[96] member and a Config struct that
    // pcMetricsStreamJob didn't carry before; it moved to heap alongside the
    // other two for exactly that reason.
    std::unique_ptr<PcMetricsStreamJob> pcMetricsStreamJob;
    std::unique_ptr<CpuClockStreamJob> cpuClockStreamJob;
    std::unique_ptr<ProcessStreamJob> processStreamJob;

    JobBundle(PlatformBundle& platform, DataBundle& data, ServiceBundle& services,
              const AppSettings& config)
        : wifiReconnectJob(platform.networkManager, data.systemState.core),
          ntpRetryJob(platform.ntpService, data.systemState.core, platform.logger_),
          pcMetricsJob(services.pcMetricsService, data.pcMetrics, data.systemState.core,
                       data.systemState.screen, platform.networkManager, config, platform.logger_),
          airQualityJob(services.airQualityService, data.airQualityData, data.systemState.core,
                        platform.networkManager, config, platform.logger_),
          roomClimateJob(services.roomClimateService, data.roomClimateData, data.systemState.core,
                         platform.networkManager, config, platform.logger_),
          networkStatusJob(services.networkStatusService, data.netStatus),
          dimAtNightJob(platform.ntpService, platform.displayManager, config),
          weatherJob(services.weatherService, data.weatherData, data.systemState.core,
                     platform.networkManager, config, platform.logger_),
          pcMetricsStreamJob(std::make_unique<PcMetricsStreamJob>(
              data.pcMetrics, data.systemState.core, data.systemState.screen,
              platform.networkManager, config, platform.logger_, services.systemMetrics)),
          cpuClockStreamJob(std::make_unique<CpuClockStreamJob>(
              *data.cpuClockData, data.systemState.core, data.systemState.screen,
              platform.networkManager, config, platform.logger_)),
          processStreamJob(std::make_unique<ProcessStreamJob>(
              *data.processData, data.systemState.core, data.systemState.screen,
              platform.networkManager, config, platform.logger_)) {}

    std::vector<BackgroundJob*> asVector() {
        return {&wifiReconnectJob,        &ntpRetryJob,
                &pcMetricsJob,            pcMetricsStreamJob.get(),
                &airQualityJob,           &roomClimateJob,
                &networkStatusJob,        &dimAtNightJob,
                &weatherJob,              cpuClockStreamJob.get(),
                processStreamJob.get()};
    }
};
