#pragma once

#include <climits>

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "utils/LoggerInterface.h"

// Refreshes air quality data whenever it goes stale, once the system is
// initialized and WiFi is up.
class AirQualityJob : public BackgroundJob {
 public:
    AirQualityJob(AirQualityService& service, AirQualityData& data,
                 SystemState::CoreState& coreState, NetworkManager& networkManager,
                 LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          logger_(logger) {}

    unsigned long nextDueMs() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() ||
            !service_.isStale(data_)) {
            return ULONG_MAX;
        }
        return 0;
    }

    void run() override {
        if (service_.fetchData(data_)) {
            logger_.debug("AirQuality data updated");
        } else {
            logger_.warning("AirQuality fetch failed");
        }
    }

 private:
    AirQualityService& service_;
    AirQualityData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    LoggerInterface& logger_;
};
