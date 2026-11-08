#pragma once

#include <WebServer.h>

#include "services/web/WebDataSources.h"
#include "services/web/WebEmitters.h"

// The /api/* JSON handlers and /metrics, split out of WebServerService so
// that class isn't also responsible for response assembly. Each handler is
// two steps (A3): captureSnapshot() does every hardware/clock/service read
// (ESP.*, WiFi.*, millis(), PcMetricsService counters) into a plain
// StatusSnapshot, then a pure WebEmitters::write*() function builds the body
// from (snapshot, feeds) — those are host-tested under [env:native]. Bodies
// are streamed via ChunkedPrint, so no large String or buffer is assembled.
class WebApiHandlers {
 public:
    WebApiHandlers(WebServer& server, const WebDataSources& sources);

    void handleApiStatus();
    void handleApiRaw();
    void handleApiPc();
    void handleMetrics();

 private:
    // The only place this class touches ESP/WiFi/the clock for /api/status
    // and /metrics.
    void captureSnapshot(StatusSnapshot& snap) const;
    // Starts a chunked, uncached response with the given content type.
    void beginChunked(const char* contentType);

    WebServer& server_;
    const WebDataSources& sources_;
};
