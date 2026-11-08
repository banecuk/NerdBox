#pragma once

#include <ArduinoJson.h>

#include <cstddef>
#include <cstdint>

#include "network/LinkInfo.h"
#include "services/web/WebDataSources.h"

#include <Print.h>

// Everything /api/status and /metrics need that comes from the hardware, the
// clock or a non-data service: captured once per request by
// WebApiHandlers::captureSnapshot() (ESP.*, WiFi.*, millis(), the 64-bit
// uptime clock, PcMetricsService's counters, NetworkStatus' link copy), so
// the emitters below are pure functions of (snapshot, feeds) and can be
// host-tested under [env:native] (A3).
struct StatusSnapshot {
    unsigned long nowMs = 0;  // millis() at capture; every *_age_ms is relative to it
    uint64_t uptimeMs = 0;    // ApplicationMetrics::uptimeMs()
    char uptime[20] = "";     // ApplicationMetrics::getFormattedUptime()

    // sys
    uint32_t heapFree = 0;
    uint32_t heapMin = 0;
    uint32_t heapMaxAlloc = 0;
    uint32_t psramSize = 0;
    uint32_t psramFree = 0;
    uint32_t cpuMhz = 0;
    const char* sdk = "";  // static storage (ESP.getSdkVersion())
    int resetReason = 0;

    // net — WiFi.localIP() while connected, else "0.0.0.0"
    char netIp[16] = "0.0.0.0";

    // pc feed
    bool pcFresh = false;  // DataFreshnessGuard(pcMetrics.freshness).isFresh()
    uint32_t pcFetchOk = 0;
    uint32_t pcFetchFail = 0;
    char pcLastError[64] = "";

    // This device's own link, copied once from NetworkStatus (A8).
    LinkInfo link = {};
};

namespace WebEmitters {

// GET /api/status body. Key names/order are a public contract (dashboard JS,
// scripts) — keep them stable.
void writeStatus(JsonDocument& doc, const StatusSnapshot& snap, const WebFeeds& feeds);

// GET /api/pc body. cpu_thread_load holds pcMetrics.cpu_core_count entries
// (clamped to Limits::kMaxThreads), not the whole backing array (B65).
void writePc(JsonDocument& doc, const WebFeeds& feeds);

// GET /metrics body (Prometheus text exposition 0.0.4). Metric names are a
// public contract (Grafana/Home Assistant) — keep them stable. Every metric
// name gets exactly one HELP/TYPE pair (B45); label values are escaped
// (B66); a feed with no data emits its HELP/TYPE but no samples (B66).
void writeMetrics(Print& out, const StatusSnapshot& snap, const WebFeeds& feeds);

// Escapes a Prometheus label value (`\` → `\\`, `"` → `\"`, newline → `\n`)
// into out (always NUL-terminated, truncated at a whole escape sequence).
// Returns the length written.
size_t escapeLabelValue(const char* in, char* out, size_t outSize);

}  // namespace WebEmitters
