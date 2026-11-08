#include "WebApiHandlers.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <WiFi.h>

#include <cstdio>

#include "services/pcMetrics/PcMetricsService.h"
#include "services/web/ChunkedPrint.h"
#include "utils/DataFreshnessGuard.h"

WebApiHandlers::WebApiHandlers(WebServer& server, const WebDataSources& sources)
    : server_(server), sources_(sources) {}

void WebApiHandlers::beginChunked(const char* contentType) {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, contentType, "");
}

// ---------------------------------------------------------------------------
// Snapshot step — every hardware, clock and non-data-service read the
// emitters need, taken once per request.
// ---------------------------------------------------------------------------

void WebApiHandlers::captureSnapshot(StatusSnapshot& snap) const {
    const WebFeeds& feeds = sources_.feeds;

    snap.nowMs = millis();
    snap.uptimeMs = ApplicationMetrics::uptimeMs();
    feeds.systemMetrics.getFormattedUptime(snap.uptime, sizeof(snap.uptime));

    snap.heapFree = ESP.getFreeHeap();
    snap.heapMin = ESP.getMinFreeHeap();
    snap.heapMaxAlloc = ESP.getMaxAllocHeap();
    snap.psramSize = ESP.getPsramSize();
    snap.psramFree = ESP.getFreePsram();
    snap.cpuMhz = ESP.getCpuFreqMHz();
    snap.sdk = ESP.getSdkVersion();
    snap.resetReason = static_cast<int>(esp_reset_reason());

    if (WiFi.status() == WL_CONNECTED) {
        IPAddress addr = WiFi.localIP();
        snprintf(snap.netIp, sizeof(snap.netIp), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    }

    snap.pcFresh = DataFreshnessGuard(feeds.pcMetrics.freshness).isFresh();
    const PcMetricsService& pcService = sources_.pcMetricsService;
    snap.pcFetchOk = pcService.getFetchOkCount();
    snap.pcFetchFail = pcService.getFetchFailCount();
    snprintf(snap.pcLastError, sizeof(snap.pcLastError), "%s", pcService.getLastError());

    feeds.netStatus.copyLink(snap.link);  // published by NetworkStatusService (A8)
}

// ---------------------------------------------------------------------------
// /api/status — JSON snapshot for scriptable monitoring / the dashboard.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiStatus() {
    StatusSnapshot snap;
    captureSnapshot(snap);

    beginChunked("application/json");
    JsonDocument doc;
    WebEmitters::writeStatus(doc, snap, sources_.feeds);

    ChunkedPrint output(server_);
    serializeJson(doc, output);
    output.flush();
    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// /api/raw — last raw NerdWinSense payload, unfiltered.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiRaw() {
    // Synchronous on the web server's (main-loop) task: every other request,
    // POST /audio included, waits until this returns — bounded by
    // HttpClient's LAN connect + response timeouts (A22; see the /api help
    // page).
    String raw;
    if (!sources_.pcMetricsService.fetchRawJson(raw)) {
        server_.send(502, "text/plain", "Failed to fetch raw payload from NerdWinSense");
        return;
    }

    // This endpoint is a deliberate exception to the "no String churn" rule —
    // it's a rarely-hit debug page, not part of the 500 ms polling cycle, and
    // the whole point is to show the payload exactly as NerdWinSense sent it.
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", raw);
}

// ---------------------------------------------------------------------------
// /api/pc — full current PcMetrics snapshot.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleApiPc() {
    beginChunked("application/json");
    JsonDocument doc;
    WebEmitters::writePc(doc, sources_.feeds);

    ChunkedPrint output(server_);
    serializeJson(doc, output);
    output.flush();
    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// /metrics — Prometheus text exposition, written straight to ChunkedPrint.
// ---------------------------------------------------------------------------

void WebApiHandlers::handleMetrics() {
    StatusSnapshot snap;
    captureSnapshot(snap);

    beginChunked("text/plain; version=0.0.4; charset=utf-8");
    ChunkedPrint out(server_);
    WebEmitters::writeMetrics(out, snap, sources_.feeds);
    out.flush();
    server_.sendContent("");  // flush / end chunked transfer
}
