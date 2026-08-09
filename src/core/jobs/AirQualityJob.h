#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/LoggerInterface.h"

// Refreshes air quality data whenever it goes stale, once the system is
// initialized and WiFi is up. Failed fetches back off for
// AppSettings::airQualityFailureBackoffMs so a wrong/expired API
// key or provider outage can't turn into a tight retry loop that starves
// other background jobs.
class AirQualityJob : public BackgroundJob {
 public:
    AirQualityJob(AirQualityService& service, AirQualityData& data,
                  SystemState::CoreState& coreState, NetworkManager& networkManager,
                  const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger),
          freshness_(data_.freshness, AirQualityService::kRefreshIntervalMs) {}

    JobDue nextDue() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() || freshness_.isFresh()) {
            return JobDue::never();
        }
        return JobDue::at(nextAttemptMs_);
    }

    void run() override {
        if (service_.fetchData(data_)) {
            logger_.debug("AirQuality data updated");
        } else {
            nextAttemptMs_ = millis() + config_.airQualityFailureBackoffMs;
            logger_.warning("AirQuality fetch failed");
        }
    }

 private:
    AirQualityService& service_;
    AirQualityData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;
    DataFreshnessGuard freshness_;
    unsigned long nextAttemptMs_ = 0;
};
