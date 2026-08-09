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

    JobDue nextDue() const override { return JobDue::now(); }

    void run() override {
        service_.updateWifi(status_);
        service_.maybeTriggerProbe(status_);
    }

 private:
    NetworkStatusService& service_;
    NetworkStatus& status_;
};
