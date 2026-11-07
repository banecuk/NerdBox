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
        // Both timers are Cooldowns (elapsed-time compare) rather than raw
        // millis() deadlines, so neither breaks after ~24.8 days of uptime
        // (B35). The failure backoff applies to both the Rescan-button and
        // the automatic path; the rescan interval only to the automatic one.
        const unsigned long now = millis();
        if (failureBackoff_.active(now)) {
            return JobDue::at(failureBackoff_.deadlineMs());
        }
        if (data_.rescanRequested.load()) {
            return JobDue::now();
        }
        return JobDue::afterCooldown(rescanInterval_, now);
    }

    void run() override {
        if (data_.state == WifiScanData::State::SCANNING) {
            if (screenState_.activeScreen == ScreenName::WIFI) {
                if (service_.poll(data_)) {
                    if (data_.state == WifiScanData::State::FAILED) {
                        failureBackoff_.start(millis(), config_.wifiScanFailureBackoffMs);
                        logger_.warning("WiFi scan failed");
                    } else {
                        failureBackoff_.clear();
                        rescanInterval_.start(millis(), config_.wifiScanRescanIntervalMs);
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

    Cooldown failureBackoff_;
    Cooldown rescanInterval_;
};
