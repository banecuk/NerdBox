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

    // Unconditional: checkAndReconnect() is a cheap early return while
    // connected, but it's also where the "link came back" branch clears
    // reconnecting_ and restarts mDNS. Gating it on !isConnected() made that
    // branch unreachable — after a rejoin the WiFi scan stayed disabled and
    // nerdbox.local stopped resolving until reboot (B36).
    void run() override { networkManager_.checkAndReconnect(); }

 private:
    NetworkManager& networkManager_;
    SystemState::CoreState& coreState_;
};
