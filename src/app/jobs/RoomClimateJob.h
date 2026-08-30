#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/roomClimate/RoomClimateData.h"
#include "services/roomClimate/RoomClimateService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/LogMacros.h"

// Refreshes the local room climate reading whenever it goes stale, once the
// system is initialized and WiFi is up. Not screen-gated: the widget lives on
// the main screen, which is the default view, and one small LAN GET per
// minute costs nothing. Direct clone of AirQualityJob.
class RoomClimateJob : public BackgroundJob {
 public:
    RoomClimateJob(RoomClimateService& service, RoomClimateData& data,
                   SystemState::CoreState& coreState, NetworkManager& networkManager,
                   const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger),
          freshness_(data_.freshness, RoomClimateService::kRefreshIntervalMs) {}

    JobDue nextDue() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() || freshness_.isFresh()) {
            return JobDue::never();
        }
        return JobDue::at(nextAttemptMs_);
    }

    void run() override {
        if (service_.fetchData(data_)) {
            LOG_DEBUG(logger_, "RoomClimate data updated");
        } else {
            nextAttemptMs_ = millis() + config_.roomClimateFailureBackoffMs;
            logger_.warning("RoomClimate fetch failed");
        }
    }

 private:
    RoomClimateService& service_;
    RoomClimateData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;
    DataFreshnessGuard freshness_;
    unsigned long nextAttemptMs_ = 0;
};
