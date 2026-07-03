#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "services/network/NetworkStatus.h"
#include "utils/LoggerInterface.h"

// Keeps NetworkStatus up to date.
//
// WiFi state  — call updateWifi() from the background task every loop tick.
//               Reads WiFi.status() and WiFi.RSSI() — no network I/O.
//
// Internet    — call maybeTriggerProbe() from the background task every tick.
//               When the probe interval has elapsed and no probe is running, a
//               one-shot FreeRTOS task is spawned that does a single HTTP HEAD
//               to one of six rotating endpoints, then self-deletes.
//               The rolling 6-slot result buffer prevents a single slow response
//               from flipping the state.
//               Until all 6 endpoints have been probed once, probing runs at
//               the fast kWarmupProbeIntervalMs cadence so the status is
//               accurate within a few seconds of boot; afterwards it settles
//               into the slow kProbeIntervalMs cadence.
//               WARNING  — exactly 1 of the 6 last results failed.
//               DEGRADED — 2 or more failed, but not all.
//               DOWN     — all 6 failed.
class NetworkStatusService {
 public:
    NetworkStatusService(LoggerInterface& logger);
    ~NetworkStatusService() = default;

    NetworkStatusService(const NetworkStatusService&) = delete;
    NetworkStatusService& operator=(const NetworkStatusService&) = delete;

    // Call from background task every loop tick.
    void updateWifi(NetworkStatus& status);

    // Call from background task every loop tick.
    // Spawns a probe task when the interval has elapsed.
    void maybeTriggerProbe(NetworkStatus& status);

    // Steady-state probe interval, used once every endpoint has been probed
    // at least once. 45s is plenty for a status glyph and cuts the request
    // volume to the (third-party) probe endpoints roughly 5.5x versus the
    // previous 8s cadence.
    static constexpr unsigned long kProbeIntervalMs = 45000UL;

    // Warm-up probe interval, used while any endpoint has never been probed
    // (i.e. right after boot). Fast enough that all kNumEndpoints slots fill
    // in a few seconds instead of waiting kNumEndpoints * kProbeIntervalMs
    // for an accurate status.
    static constexpr unsigned long kWarmupProbeIntervalMs = 1000UL;

    static constexpr uint8_t kNumEndpoints = 6;

 private:
    // -----------------------------------------------------------------------
    // Probe task
    // -----------------------------------------------------------------------
    struct ProbeContext {
        NetworkStatusService* service;
        NetworkStatus* status;
    };

    static void probeTaskEntry(void* param);
    void runProbe(NetworkStatus& status);

    // Records a single probe result (1=pass, 0=fail) into the rolling buffer
    // and recomputes status_.internet.
    void recordResult(NetworkStatus& status, uint8_t endpointIdx, bool success);

    // True once every endpoint has been probed at least once.
    bool allEndpointsProbed() const;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    LoggerInterface& logger_;

    uint8_t probeTarget_ = 0;  // round-robin index into kProbeUrls
    uint8_t results_[kNumEndpoints] = {0, 0, 0, 0,
                                       0, 0};  // rolling pass/fail buffer (one slot per endpoint)
    // Tracks which slots have received at least one real result. Until every
    // slot has been probed once (kNumEndpoints * kProbeIntervalMs after boot),
    // recordResult() must only weigh the slots that have actually been probed
    // — otherwise never-probed slots default to "fail" and the status is
    // misreported as DEGRADED/DOWN during the warm-up window.
    bool probed_[kNumEndpoints] = {false, false, false, false, false, false};

    static constexpr uint32_t kProbeStack = 4096;
    static constexpr uint32_t kProbeTimeoutMs = 1500;
    static constexpr UBaseType_t kProbePriority = 1;

    static const char* kProbeUrls[kNumEndpoints];
};
