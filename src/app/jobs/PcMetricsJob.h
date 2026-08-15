#pragma once

#include <Arduino.h>

#include <atomic>

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/LogMacros.h"

// Periodically fetches PC hardware metrics while a screen that displays them
// (main or game) is active. Owns the retry/backoff and consecutive-failure
// bookkeeping that used to live directly in TaskManager.
//
// As of SSE-PUSH-PLAN.md milestone 6, PcMetricsStreamJob (SSE) is the
// default data path (AppSettings.pcMetricsStreamEnabled = true) and this
// job is the deliberately-retained fallback — see nextDue() below. Kept
// in the tree for at least one release in case streaming misbehaves in
// the field; set pcMetricsStreamEnabled back to false to fall back to
// this path with no code change. Slated for removal once streaming has
// proven stable over a longer soak (not yet done — see the plan).
class PcMetricsJob : public BackgroundJob {
 public:
    PcMetricsJob(PcMetricsService& service, PcMetrics& metrics, SystemState::CoreState& coreState,
                 SystemState::ScreenState& screenState, NetworkManager& networkManager,
                 const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          metrics_(metrics),
          coreState_(coreState),
          screenState_(screenState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger),
          freshness_(metrics_.freshness) {}

    JobDue nextDue() const override {
        // Mutually exclusive with PcMetricsStreamJob — when streaming is
        // enabled it is the sole writer of PcMetrics, so polling never
        // becomes due (see SSE-PUSH-PLAN.md milestone 4).
        if (config_.pcMetricsStreamEnabled) {
            return JobDue::never();
        }
        const bool onMetricsScreen = screenState_.activeScreen == ScreenName::MAIN ||
                                     screenState_.activeScreen == ScreenName::GAME ||
                                     screenState_.activeScreen == ScreenName::DISKS;
        if (!coreState_.isInitialized || !onMetricsScreen || !networkManager_.isConnected()) {
            return JobDue::never();
        }
        return JobDue::at(nextSync_);
    }

    void run() override {
        if (service_.fetchData(metrics_)) {
            consecutiveFailures_ = 0;
            nextSync_ = millis() + config_.hardwareMonitorRefreshMs;
            return;
        }

        consecutiveFailures_++;
        nextSync_ = millis() + config_.hardwareMonitorFailureRefreshMs;
        LOG_DEBUG(logger_, "PC metrics update failed", true);

        if (consecutiveFailures_ >= config_.hardwareMonitorMaxRetries) {
            logger_.warning("Multiple consecutive PC metrics failures detected", true);
            consecutiveFailures_ = 0;
        }

        if (!freshness_.isFresh()) {
            logger_.warning("PC metrics data is stale", true);
        }
    }

 private:
    PcMetricsService& service_;
    PcMetrics& metrics_;
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    // Single source of truth for PC-metrics staleness — shared with
    // PcMetricsWidget via the same PcMetrics::freshness field.
    DataFreshnessGuard freshness_;
    uint8_t consecutiveFailures_ = 0;
    unsigned long nextSync_ = 0;
};
