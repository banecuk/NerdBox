#include "NetworkStatusService.h"

#include <HTTPClient.h>
#include <WiFi.h>

// Six lightweight connectivity-check endpoints, tried in rotation.
// All return quickly and are operated by highly reliable providers.
const char* NetworkStatusService::kProbeUrls[NetworkStatusService::kNumEndpoints] = {
    "http://connectivitycheck.gstatic.com/generate_204",  // Google         — expect 204
    "http://www.msftncsi.com/ncsi.txt",                   // Microsoft NCSI — expect 200
    "http://captive.apple.com/hotspot-detect.html",       // Apple          — expect 200
    "http://connectivitycheck.android.com/generate_204",  // Android AOSP   — expect 204
    "http://clients3.google.com/generate_204",            // Google alt     — expect 204
    "http://nmcheck.gnome.org/check_network_status.txt",  // GNOME          — expect 200
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkStatusService::NetworkStatusService(LoggerInterface& logger) : logger_(logger) {}

// ---------------------------------------------------------------------------
// updateWifi — cheap, called every background loop tick
// ---------------------------------------------------------------------------

void NetworkStatusService::updateWifi(NetworkStatus& status) {
    status.wifi_connected = (WiFi.status() == WL_CONNECTED);
    if (status.wifi_connected) {
        status.rssi = static_cast<int8_t>(WiFi.RSSI());
    } else {
        status.rssi = 0;
        status.internet = NetworkStatus::Internet::UNKNOWN;
    }
}

// ---------------------------------------------------------------------------
// maybeTriggerProbe — spawns a one-shot task when the interval has elapsed
// ---------------------------------------------------------------------------

void NetworkStatusService::maybeTriggerProbe(NetworkStatus& status) {
    if (!status.wifi_connected)
        return;
    if (status.probe_running)
        return;
    if (status.last_probe != 0 && (millis() - status.last_probe) < kProbeIntervalMs)
        return;

    // Allocate context on the heap — freed by the probe task before self-delete.
    auto* ctx = new ProbeContext{this, &status};
    status.probe_running = true;

    BaseType_t ok = xTaskCreatePinnedToCore(
        probeTaskEntry, "net_probe", kProbeStack, ctx, kProbePriority, nullptr,
        0  // core 0 — same as background task, away from display core
    );

    if (ok != pdPASS) {
        logger_.warning("NetworkStatus: failed to spawn probe task");
        delete ctx;
        status.probe_running = false;
    }
}

// ---------------------------------------------------------------------------
// probeTaskEntry — static trampoline; runs on core 0, self-deletes on exit
// ---------------------------------------------------------------------------

void NetworkStatusService::probeTaskEntry(void* param) {
    auto* ctx = static_cast<ProbeContext*>(param);
    ctx->service->runProbe(*ctx->status);
    delete ctx;
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// runProbe — executed inside the short-lived probe task
// ---------------------------------------------------------------------------

void NetworkStatusService::runProbe(NetworkStatus& status) {
    const uint8_t idx = probeTarget_;
    const char* url = kProbeUrls[idx];
    probeTarget_ = (probeTarget_ + 1) % kNumEndpoints;

    bool success = false;

    HTTPClient http;
    http.setTimeout(kProbeTimeoutMs);
    http.setConnectTimeout(static_cast<int>(kProbeTimeoutMs));

    if (http.begin(url)) {
        const int code = http.GET();
        // Accept any 2xx response — 200 or 204
        success = (code >= 200 && code < 300);
        logger_.debugf("NetProbe: %s -> %d (%s)", url, code, success ? "OK" : "FAIL");
        http.end();
    } else {
        logger_.warningf("NetProbe: http.begin failed for %s", url);
    }

    recordResult(status, idx, success);

    status.last_probe = millis();
    status.probe_running = false;
}

// ---------------------------------------------------------------------------
// recordResult — update per-endpoint slot and recompute internet state
//
// WARNING  — exactly 1 of the 6 slots failed
// DEGRADED — 2 or more failed, but not all 6
// DOWN     — all 6 failed
// OK       — all 6 passed
// ---------------------------------------------------------------------------

void NetworkStatusService::recordResult(NetworkStatus& status, uint8_t endpointIdx, bool success) {
    results_[endpointIdx] = success ? 1u : 0u;
    status.endpoint_ok[endpointIdx] = success;

    uint8_t passes = 0;
    for (uint8_t i = 0; i < kNumEndpoints; ++i) {
        passes += results_[i];
    }

    const uint8_t failures = kNumEndpoints - passes;

    if (failures == 0) {
        status.internet = NetworkStatus::Internet::OK;
    } else if (failures == 1) {
        status.internet = NetworkStatus::Internet::WARNING;
    } else if (failures < kNumEndpoints) {
        status.internet = NetworkStatus::Internet::DEGRADED;
    } else {
        status.internet = NetworkStatus::Internet::DOWN;
    }
}
