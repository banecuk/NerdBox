#pragma once

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"

// Retries a dropped WiFi connection. Safe to call every background-task
// tick — NetworkManager internally rate-limits reconnect attempts.
class WifiReconnectJob : public BackgroundJob {
 public:
    WifiReconnectJob(NetworkManager& networkManager, SystemState::CoreState& coreState)
        : networkManager_(networkManager), coreState_(coreState) {}

    JobDue nextDue() const override {
        return coreState_.isInitialized ? JobDue::now() : JobDue::never();
    }

    void run() override {
        if (!networkManager_.isConnected()) {
            networkManager_.checkAndReconnect();
        }
    }

 private:
    NetworkManager& networkManager_;
    SystemState::CoreState& coreState_;
};
