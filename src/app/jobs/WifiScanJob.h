#pragma once

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/ScreenTypes.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"
#include "services/wifiScan/WifiScanData.h"
#include "services/wifiScan/WifiScanService.h"
#include "utils/logging/LoggerInterface.h"
#include "utils/logging/LogMacros.h"

// Drives WifiScanService's async scan state machine, screen-gated to the
// WIFI screen. See docs-local/13-wifi-screen-plan.md §3.3-3.4 — the
// in-flight check sits *above* the screen gate in nextDue() so a scan
// started while the screen was open still gets polled (and its result
// drained) after the user navigates away; an async scan cannot be cancelled,
// only abandoned once it settles.
class WifiScanJob : public BackgroundJob {
 public:
    WifiScanJob(WifiScanService& service, WifiScanData& data, SystemState::CoreState& coreState,
               SystemState::ScreenState& screenState, NetworkManager& networkManager,
               const AppSettings& config, LoggerInterface& logger)
        : service_(service),
          data_(data),
          coreState_(coreState),
          screenState_(screenState),
          networkManager_(networkManager),
          config_(config),
          logger_(logger) {}

    JobDue nextDue() const override {
        if (data_.state == WifiScanData::State::SCANNING) {
            return JobDue::now();
        }
        if (!coreState_.isInitialized) {
            return JobDue::never();
        }
        if (screenState_.activeScreen != ScreenName::WIFI) {
            return JobDue::never();
        }
        if (networkManager_.isReconnecting()) {
            return JobDue::never();
        }
        if (data_.rescanRequested.load()) {
            return JobDue::at(nextAttemptMs_);  // honours an active failure backoff
        }
        return JobDue::at(lastScanCompletedMs_ + config_.wifiScanRescanIntervalMs);
    }

    void run() override {
        if (data_.state == WifiScanData::State::SCANNING) {
            if (screenState_.activeScreen == ScreenName::WIFI) {
                if (service_.poll(data_)) {
                    if (data_.state == WifiScanData::State::FAILED) {
                        nextAttemptMs_ = millis() + config_.wifiScanFailureBackoffMs;
                        logger_.warning("WiFi scan failed");
                    } else {
                        lastScanCompletedMs_ = millis();
                    }
                }
            } else {
                service_.abandon(data_);
            }
            return;
        }

        data_.rescanRequested.store(false);
        service_.start(data_);
    }

 private:
    WifiScanService& service_;
    WifiScanData& data_;
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    NetworkManager& networkManager_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    unsigned long nextAttemptMs_ = 0;
    unsigned long lastScanCompletedMs_ = 0;
};
