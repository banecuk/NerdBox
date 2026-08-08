#pragma once

#include <climits>

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "network/NetworkManager.h"

// Retries a dropped WiFi connection. Safe to call every background-task
// tick — NetworkManager internally rate-limits reconnect attempts.
class WifiReconnectJob : public BackgroundJob {
 public:
    WifiReconnectJob(NetworkManager& networkManager, SystemState::CoreState& coreState)
        : networkManager_(networkManager), coreState_(coreState) {}

    unsigned long nextDueMs() const override { return coreState_.isInitialized ? 0 : ULONG_MAX; }

    void run() override {
        if (!networkManager_.isConnected()) {
            networkManager_.checkAndReconnect();
        }
    }

 private:
    NetworkManager& networkManager_;
    SystemState::CoreState& coreState_;
};
