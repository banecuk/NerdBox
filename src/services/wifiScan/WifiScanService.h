#pragma once

#include "config/AppSettings.h"
#include "services/wifiScan/WifiScanData.h"
#include "utils/logging/LoggerInterface.h"

// Async WiFi scan driver — the only class that includes <WiFi.h> for
// scanning. WifiScanJob drives this state machine on the background task,
// screen-gated to the WIFI screen (see docs-local/13-wifi-screen-plan.md
// §3.1, §3.4).
class WifiScanService {
 public:
    WifiScanService(LoggerInterface& logger, const AppSettings& config);

    // Kicks off an async scan. Returns false if one is already running.
    bool start(WifiScanData& data);

    // Polls an in-flight scan. Returns true once it has completed (ok or
    // failed) — data.state/entries/count/etc. are updated in place either
    // way. Returns false (no-op) while still running.
    bool poll(WifiScanData& data);

    // Screen left mid-scan: an async scan cannot be cancelled, only drained.
    // Checks WiFi.scanComplete() and, once it stops reporting "running",
    // frees the driver's result list via WiFi.scanDelete() and returns the
    // job to IDLE. A no-op while the scan is still genuinely in flight — the
    // caller keeps calling this every tick until it settles.
    void abandon(WifiScanData& data);

 private:
    LoggerInterface& logger_;
    const AppSettings& config_;
    unsigned long scanStartMs_ = 0;

    void applyResults(WifiScanData& data, int16_t resultCount);
};
