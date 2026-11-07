#pragma once

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/LogMacros.h"

// Refreshes one JsonHttpService-backed data struct whenever it goes stale,
// once the system is initialized and WiFi is up. Failed fetches back off for
// `failureBackoffMs` so a wrong/expired API key or provider outage can't turn
// into a tight retry loop that starves other background jobs.
//
// Not screen-gated — fits feeds whose widget lives on the main screen (the
// default view) and that cost one small GET per refresh interval.
// Instantiated as AirQualityJob and RoomClimateJob (see JobBundle.h), which
// used to be byte-level hand-copies of each other.
//
// TService must expose `static constexpr unsigned long kRefreshIntervalMs` and
// `bool fetchData(TData&)`; TData must expose a `PublishedFlag freshness`.
//
// WeatherJob is close to this shape but has its own refreshRequested fast
// path and screen gating, so it stays separate.
//
// The failure backoff is a Cooldown (elapsed-time, wrap-safe) rather than an
// absolute deadline that only failures refresh — see B35.
template <typename TService, typename TData>
class PeriodicFetchJob : public BackgroundJob {
 public:
    PeriodicFetchJob(TService& service, TData& data, SystemState::CoreState& coreState,
                     NetworkManager& networkManager, LoggerInterface& logger, const char* name,
                     uint32_t failureBackoffMs)
        : service_(service),
          data_(data),
          coreState_(coreState),
          networkManager_(networkManager),
          logger_(logger),
          name_(name),
          failureBackoffMs_(failureBackoffMs),
          freshness_(data_.freshness, TService::kRefreshIntervalMs) {}

    JobDue nextDue() const override {
        if (!coreState_.isInitialized || !networkManager_.isConnected() || freshness_.isFresh()) {
            return JobDue::never();
        }
        return JobDue::afterCooldown(backoff_, millis());
    }

    void run() override {
        backoff_.clear();
        if (service_.fetchData(data_)) {
            LOG_DEBUGF(logger_, "%s data updated", name_);
        } else {
            backoff_.start(millis(), failureBackoffMs_);
            logger_.warningf("%s fetch failed", name_);
        }
    }

 private:
    TService& service_;
    TData& data_;
    SystemState::CoreState& coreState_;
    NetworkManager& networkManager_;
    LoggerInterface& logger_;
    const char* name_;
    uint32_t failureBackoffMs_;
    DataFreshnessGuard freshness_;
    Cooldown backoff_;
};
