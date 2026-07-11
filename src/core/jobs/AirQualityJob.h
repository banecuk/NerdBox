#pragma once

#include <climits>

#include "config/AppConfigInterface.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "utils/LoggerInterface.h"

// Refreshes air quality data whenever it goes stale, once the system is
// initialized and WiFi is up. Failed fetches back off for
// AppConfigInterface::getAirQualityFailureBackoffMs() so a wrong/expired API
// key or provider outage can't turn into a tight retry loop that starves
// other background jobs.
class AirQualityJob : public BackgroundJob {
 public:
    AirQualityJob(AirQualityService& service, AirQualityData& data,
                 SystemState::CoreState& coreState, NetworkManager& networkManager,
                 AppConfigInterface& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger) {}

    unsigned long nextDueMs() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() ||
            !service_.isStale(data_)) {
            return ULONG_MAX;
        }
        return nextAttemptMs_;
    }

    void run() override {
        if (service_.fetchData(data_)) {
            logger_.debug("AirQuality data updated");
        } else {
            nextAttemptMs_ = millis() + config_.getAirQualityFailureBackoffMs();
            logger_.warning("AirQuality fetch failed");
        }
    }

 private:
    AirQualityService& service_;
    AirQualityData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    AppConfigInterface& config_;
    LoggerInterface& logger_;
    unsigned long nextAttemptMs_ = 0;
};
