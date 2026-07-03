#pragma once

#include "core/BackgroundJob.h"
#include "services/network/NetworkStatus.h"
#include "services/network/NetworkStatusService.h"

// Keeps WiFi/internet-reachability status current. Runs every tick
// unconditionally — NetworkStatusService internally rate-limits probes.
class NetworkStatusJob : public BackgroundJob {
 public:
    NetworkStatusJob(NetworkStatusService& service, NetworkStatus& status)
        : service_(service), status_(status) {}

    unsigned long nextDueMs() const override { return 0; }

    void run() override {
        service_.updateWifi(status_);
        service_.maybeTriggerProbe(status_);
    }

 private:
    NetworkStatusService& service_;
    NetworkStatus& status_;
};
