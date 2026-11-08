#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config/AppSettings.h"
#include "config/Limits.h"
#include "network/NetworkManager.h"
#include "services/network/InternetVerdict.h"
#include "services/network/NetworkStatus.h"
#include "utils/logging/LoggerInterface.h"

// Keeps NetworkStatus up to date.
//
// WiFi state  — call updateWifi() from the background task every loop tick.
//               Reads WiFi.status() and WiFi.RSSI() — no network I/O. Also
//               republishes the link details (NetworkStatus::copyLink()) at
//               most every networkLinkRefreshMs, and at once when the link goes up
//               or down, so no other task has to query the WiFi driver (A8).
//
// Internet    — call maybeTriggerProbe() from the background task every tick.
//               When the probe interval has elapsed and no probe is running,
//               the persistent probe task (created once, on the first probe)
//               is woken via a task notification and does a single HTTP GET
//               to one of eight rotating endpoints, then sleeps again. The
//               GET runs off the background task so a slow endpoint can never
//               stall it past the watchdog. No per-probe task or heap
//               allocation (A21).
//               The rolling 8-slot result buffer prevents a single slow response
//               from flipping the state.
//               Until all 8 endpoints have been probed once, probing runs at
//               the fast networkWarmupProbeIntervalMs cadence so the status is
//               accurate within a few seconds of boot; afterwards it settles
//               into the slow networkProbeIntervalMs cadence. (Both are AppSettings knobs.)
//               OK       — at most 1 of the 8 last results failed. A single
//                          third-party endpoint being flaky says nothing about
//                          this device's connectivity, so it isn't flagged.
//               WARNING  — exactly 2 failed.
//               DEGRADED — 3 or more failed, but not all.
//               DOWN     — all 8 failed.
class NetworkStatusService {
 public:
    NetworkStatusService(LoggerInterface& logger, const NetworkManager& networkManager,
                         const AppSettings& config);
    // Deletes the probe task if one was created. In practice the service
    // lives for the whole program (ServiceBundle); this only stops the task
    // from outliving `this`.
    ~NetworkStatusService();

    NetworkStatusService(const NetworkStatusService&) = delete;
    NetworkStatusService& operator=(const NetworkStatusService&) = delete;

    // Call from background task every loop tick.
    void updateWifi(NetworkStatus& status);

    // Call from background task every loop tick.
    // Wakes the probe task when the interval has elapsed.
    void maybeTriggerProbe(NetworkStatus& status);

    static constexpr uint8_t kNumEndpoints = AppConfig::Limits::kNetworkProbeEndpoints;
    static_assert(sizeof(NetworkStatus{}.endpoint_ok) / sizeof(bool) == kNumEndpoints,
                  "NetworkStatus::endpoint_ok must have exactly kNumEndpoints slots");

    // URL for a given probe slot, matching NetworkStatus::endpoint_ok[]'s
    // indexing. Returns nullptr if index is out of range.
    static const char* probeUrl(uint8_t index) {
        return index < kNumEndpoints ? kProbeUrls[index] : nullptr;
    }

 private:
    // -----------------------------------------------------------------------
    // Probe task
    // -----------------------------------------------------------------------
    // What the probe task works on. Written by the background task only while
    // NetworkStatus::probe_running is false (i.e. the probe task is parked in
    // ulTaskNotifyTake), and read by the probe task only after it is woken —
    // so it is never rewritten mid-probe. xTaskNotifyGive's critical section
    // orders the write before the wake.
    struct ProbeContext {
        NetworkStatus* status = nullptr;
    };

    // Creates the probe task if it doesn't exist yet. Returns false (and
    // logs) if creation failed; the next due probe retries.
    bool ensureProbeTask();

    // Task body: block on the notification, run one probe, repeat. The
    // parameter is `this`.
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
    const NetworkManager& networkManager_;
    const AppSettings& config_;

    unsigned long lastLinkRefreshMs_ = 0;
    bool linkPublished_ = false;
    bool lastLinkConnected_ = false;

    uint8_t probeTarget_ = 0;  // round-robin index into kProbeUrls
    // Rolling pass/fail buffer, one slot per endpoint. `probed` tracks which
    // slots have received at least one real result — until every slot has
    // been probed once, only the probed ones count towards the verdict
    // (see classifyInternet), otherwise never-probed slots would read as
    // failures and misreport DEGRADED/DOWN during the warm-up window.
    ProbeResult probes_[kNumEndpoints] = {};

    TaskHandle_t probeTask_ = nullptr;
    ProbeContext probeCtx_;

    // Not tuning knobs: the probe task's stack is sized for HTTPClient, and
    // its priority must stay at or below the background task's. The task is
    // deliberately not registered with the task watchdog: it spends almost
    // all its life blocked in ulTaskNotifyTake (portMAX_DELAY), and each
    // HTTP GET is bounded by networkProbeTimeoutMs anyway.
    static constexpr uint32_t kProbeStack = 4096;
    static constexpr UBaseType_t kProbePriority = 1;

    static const char* kProbeUrls[kNumEndpoints];
};
