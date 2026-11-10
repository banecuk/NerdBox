#include "NetworkStatusService.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "utils/logging/LogMacros.h"

// Eight lightweight connectivity-check endpoints, tried in rotation.
// All return quickly and are operated by highly reliable providers.
const char* NetworkStatusService::kProbeUrls[NetworkStatusService::kNumEndpoints] = {
    "http://connectivitycheck.gstatic.com/generate_204",  // Google        — expect 204
    "http://www.msftncsi.com/ncsi.txt",                   // Microsoft NCSI — expect 200
    "http://captive.apple.com/hotspot-detect.html",       // Apple          — expect 200
    "http://connectivitycheck.android.com/generate_204",  // Android AOSP   — expect 204
    "http://clients3.google.com/generate_204",            // Google alt     — expect 204
    "http://nmcheck.gnome.org/check_network_status.txt",  // GNOME          — expect 200
    "http://detectportal.firefox.com/success.txt",        // Mozilla        — expect 200
    "http://cp.cloudflare.com/generate_204",              // Cloudflare     — expect 204
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

NetworkStatusService::NetworkStatusService(LoggerInterface& logger,
                                           const NetworkManager& networkManager,
                                           const AppSettings& config)
    : logger_(logger), networkManager_(networkManager), config_(config) {}

NetworkStatusService::~NetworkStatusService() {
    if (probeTask_ != nullptr) {
        vTaskDelete(probeTask_);
        probeTask_ = nullptr;
    }
}

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

    const unsigned long now = millis();
    const bool edge = status.wifi_connected != lastLinkConnected_;
    if (!linkPublished_ || edge || now - lastLinkRefreshMs_ >= config_.networkLinkRefreshMs) {
        LinkInfo link;
        networkManager_.fillLinkInfo(link);
        status.storeLink(link, now);
        lastLinkRefreshMs_ = now;
        lastLinkConnected_ = status.wifi_connected;
        linkPublished_ = true;
    }
}

// ---------------------------------------------------------------------------
// maybeTriggerProbe — wakes the persistent probe task when the interval has
// elapsed
// ---------------------------------------------------------------------------

void NetworkStatusService::maybeTriggerProbe(NetworkStatus& status) {
    if (!status.wifi_connected)
        return;
    // A probe still in flight (slow endpoint) when the next one falls due:
    // skip, don't queue. The interval is measured from that probe's
    // completion (last_probe), so the next one simply starts later.
    if (status.probe_running)
        return;

    const unsigned long interval = allEndpointsProbed() ? config_.networkProbeIntervalMs
                                                        : config_.networkWarmupProbeIntervalMs;
    if (status.last_probe != 0 && (millis() - status.last_probe) < interval)
        return;

    if (!ensureProbeTask()) {
        // Back off a full interval rather than retrying (and logging) on every
        // 20 ms background tick.
        status.last_probe = millis();
        return;
    }

    // The probe task is parked (probe_running was false), so the context is
    // safe to write. Set probe_running before the wake: the probe task clears
    // it when done, and nothing rewrites probeCtx_ or notifies again until then.
    probeCtx_.status = &status;
    status.probe_running = true;
    xTaskNotifyGive(probeTask_);
}

// ---------------------------------------------------------------------------
// ensureProbeTask — creates the persistent probe task once, lazily
// ---------------------------------------------------------------------------

bool NetworkStatusService::ensureProbeTask() {
    if (probeTask_ != nullptr)
        return true;

    const BaseType_t ok = xTaskCreatePinnedToCore(
        probeTaskEntry, "net_probe", kProbeStack, this, kProbePriority, &probeTask_,
        0  // core 0 — same as background task, away from display core
    );
    if (ok != pdPASS) {
        probeTask_ = nullptr;
        logger_.warning("NetworkStatus: failed to create probe task");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// probeTaskEntry — persistent task body on core 0: sleep until notified, run
// one probe, repeat. Never returns.
// ---------------------------------------------------------------------------

void NetworkStatusService::probeTaskEntry(void* param) {
    auto* self = static_cast<NetworkStatusService*>(param);
    for (;;) {
        // pdTRUE clears the count, so any duplicate notification collapses
        // into a single probe rather than queueing a backlog.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        NetworkStatus* status = self->probeCtx_.status;
        if (status != nullptr) {
            self->runProbe(*status);
        }
    }
}

// ---------------------------------------------------------------------------
// runProbe — executed on the persistent probe task
// ---------------------------------------------------------------------------

void NetworkStatusService::runProbe(NetworkStatus& status) {
    const uint8_t idx = probeTarget_;
    const char* url = kProbeUrls[idx];
    probeTarget_ = (probeTarget_ + 1) % kNumEndpoints;

    bool success = false;

    HTTPClient http;
    http.setTimeout(config_.networkProbeTimeoutMs);
    http.setConnectTimeout(static_cast<int>(config_.networkProbeTimeoutMs));

    if (http.begin(url)) {
        const int code = http.GET();
        // Accept any 2xx response — 200 or 204
        success = (code >= 200 && code < 300);
        LOG_DEBUGF(logger_, "NetProbe: %s -> %d (%s)", url, code, success ? "OK" : "FAIL");
        http.end();
    } else {
        logger_.warningf("NetProbe: http.begin failed for %s", url);
    }

    recordResult(status, idx, success);

    status.last_probe = millis();
    status.probe_running = false;
}

// ---------------------------------------------------------------------------
// recordResult — update per-endpoint slot and recompute internet state.
// The verdict rules live in classifyInternet() (InternetVerdict.h).
// ---------------------------------------------------------------------------

void NetworkStatusService::recordResult(NetworkStatus& status, uint8_t endpointIdx, bool success) {
    probes_[endpointIdx].ok = success;
    probes_[endpointIdx].probed = true;
    status.endpoint_ok[endpointIdx] = success;
    status.endpoint_probed[endpointIdx] = true;
    status.internet = classifyInternet(probes_, kNumEndpoints);
}

bool NetworkStatusService::allEndpointsProbed() const {
    for (uint8_t i = 0; i < kNumEndpoints; ++i) {
        if (!probes_[i].probed)
            return false;
    }
    return true;
}
