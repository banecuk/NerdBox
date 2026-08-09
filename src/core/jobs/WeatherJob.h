#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/weather/WeatherData.h"
#include "services/weather/WeatherService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/LoggerInterface.h"

// Refreshes the open-meteo weather forecast every kWeatherRefreshIntervalMs
// (2 h), regardless of which screen is active, so data is already current
// whenever the user opens the Weather screen. Fires immediately, ahead of
// that interval, when refreshRequested is set — on screen entry (UiController)
// or on a local-midnight rollover (WeatherWidget). Failed fetches back off
// for AppSettings::weatherFailureBackoffMs.
class WeatherJob : public BackgroundJob {
 public:
    WeatherJob(WeatherService& service, WeatherData& data, SystemState::CoreState& coreState,
               NetworkManager& networkManager, const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger),
          freshness_(data_.freshness, config_.weatherRefreshIntervalMs) {}

    JobDue nextDue() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected()) {
            return JobDue::never();
        }
        if (data_.refreshRequested.load() && millis() >= nextAttemptMs_) {
            return JobDue::now();  // screen entry / midnight rollover → fire now (still honours failure backoff)
        }
        if (freshness_.isFresh()) {
            return JobDue::never();  // fetched < 2 h ago → skip
        }
        return JobDue::at(nextAttemptMs_);
    }

    void run() override {
        if (service_.fetchData(data_)) {
            logger_.debug("Weather data updated");
        } else {
            nextAttemptMs_ = millis() + config_.weatherFailureBackoffMs;
            logger_.warning("Weather fetch failed");
        }
    }

 private:
    WeatherService& service_;
    WeatherData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;
    DataFreshnessGuard freshness_;
    unsigned long nextAttemptMs_ = 0;
};
