#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/weather/WeatherData.h"
#include "services/weather/WeatherService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/LoggerInterface.h"

// Refreshes the open-meteo weather forecast. Unlike the always-running jobs,
// this one only ever fires while the Weather screen is the active screen —
// "don't refetch unless displayed" (see docs/07-weather-forecast.md). While
// displayed it re-fetches when data goes stale (2 h) or immediately when the
// screen task signals a local-midnight rollover via refreshRequested.
// Failed fetches back off for AppSettings::weatherFailureBackoffMs.
class WeatherJob : public BackgroundJob {
 public:
    WeatherJob(WeatherService& service, WeatherData& data, SystemState::ScreenState& screenState,
               SystemState::CoreState& coreState, NetworkManager& networkManager,
               const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          screenState_(screenState),
          coreState_(coreState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger),
          freshness_(data_.freshness, config_.weatherRefreshIntervalMs) {}

    JobDue nextDue() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() ||
            screenState_.activeScreen != ScreenName::WEATHER) {
            return JobDue::never();  // not displayed → never due
        }
        if (data_.refreshRequested.load() && millis() >= nextAttemptMs_) {
            return JobDue::now();  // midnight rollover → fire now (still honours failure backoff)
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
    SystemState::ScreenState& screenState_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;
    DataFreshnessGuard freshness_;
    unsigned long nextAttemptMs_ = 0;
};
