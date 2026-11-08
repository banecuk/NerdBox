#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/weather/WeatherData.h"
#include "services/weather/WeatherDayMath.h"
#include "services/weather/WeatherService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/LogMacros.h"
#include "utils/ScopedLock.h"

// Refreshes the open-meteo weather forecast every kWeatherRefreshIntervalMs
// (2 h), regardless of which screen is active, so data is already current
// whenever the user opens the Weather screen. Fires immediately, ahead of
// that interval, when refreshRequested is set (on Weather screen entry —
// WeatherScreen::onEnter()) or when the forecast's first day is no longer
// today (a local-midnight rollover, checked here every
// weatherTimeCheckIntervalMs via WeatherDayMath — A11). Failed fetches back
// off for AppSettings::weatherFailureBackoffMs.
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
        checkMidnightRollover();
        // Screen entry / midnight rollover → fire now, still honouring the
        // failure backoff. Otherwise only refresh once the data is stale.
        // Cooldown compares elapsed time, so neither path breaks at the
        // millis() rollover (B7/B35).
        if (!data_.refreshRequested.load() && freshness_.isFresh()) {
            return JobDue::never();  // fetched < 2 h ago → skip
        }
        return JobDue::afterCooldown(backoff_, millis());
    }

    void run() override {
        backoff_.clear();
        // Consume the request as the fetch starts, so a new one raised while
        // this fetch is in flight survives for the next run (B60).
        const bool wasRequested = data_.refreshRequested.exchange(false);
        if (service_.fetchData(data_)) {
            LOG_DEBUG(logger_, "Weather data updated");
        } else {
            // Keep a consumed request pending so it retries after the backoff
            // instead of waiting for the 2 h staleness refresh.
            if (wasRequested) {
                data_.refreshRequested.store(true);
            }
            backoff_.start(millis(), config_.weatherFailureBackoffMs);
            logger_.warning("Weather fetch failed");
        }
    }

 private:
    // Throttled to weatherTimeCheckIntervalMs (nextDue() runs every
    // background tick). Raises refreshRequested rather than returning a
    // separate verdict, so the request survives a failed fetch exactly like
    // a screen-entry one (run() re-arms it). Mutable/const because
    // nextDue() is the scheduler's only hook; it touches nothing but the
    // throttle stamp and the shared request flag.
    void checkMidnightRollover() const {
        const unsigned long now = millis();
        if (now - lastTimeCheckMs_ < config_.weatherTimeCheckIntervalMs) {
            return;
        }
        lastTimeCheckMs_ = now;

        if (!coreState_.isTimeSynced || data_.dayCount == 0) {
            return;
        }
        struct tm nowLocal;
        if (!getLocalTime(&nowLocal, 0)) {
            return;
        }
        time_t firstDayStart;
        {
            ScopedLock lock(data_.daysMutex);
            firstDayStart = data_.days[0].dayStart;
        }
        if (WeatherDayMath::needsRollover(nowLocal, firstDayStart)) {
            data_.refreshRequested.store(true);
        }
    }

    WeatherService& service_;
    WeatherData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;
    DataFreshnessGuard freshness_;
    Cooldown backoff_;
    mutable unsigned long lastTimeCheckMs_ = 0;
};
