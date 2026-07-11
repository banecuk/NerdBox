#pragma once

#include <Arduino.h>

#include <climits>

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/LoggerInterface.h"

// Periodically fetches PC hardware metrics while the main screen is active.
// Owns the retry/backoff and consecutive-failure bookkeeping that used to
// live directly in TaskManager.
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
          freshness_(metrics_.is_available, metrics_.last_update_timestamp) {}

    unsigned long nextDueMs() const override {
        if (!coreState_.isInitialized || screenState_.activeScreen != ScreenName::MAIN ||
            !networkManager_.isConnected()) {
            return ULONG_MAX;
        }
        return coreState_.nextSync_pcMetrics;
    }

    void run() override {
        if (service_.fetchData(metrics_)) {
            consecutiveFailures_ = 0;
            coreState_.nextSync_pcMetrics = millis() + config_.hardwareMonitorRefreshMs;
            return;
        }

        consecutiveFailures_++;
        coreState_.nextSync_pcMetrics = millis() + config_.hardwareMonitorFailureRefreshMs;
        logger_.debug("PC metrics update failed", true);

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
    // PcMetricsWidget via the same PcMetrics::last_update_timestamp field.
    DataFreshnessGuard freshness_;
    uint8_t consecutiveFailures_ = 0;
};
