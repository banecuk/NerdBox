#include "WebPageHandlers.h"

#include <cstring>
#include <memory>

#include "services/web/WebAssets.h"

WebPageHandlers::WebPageHandlers(WebServer& server, const WebDataSources& sources)
    : server_(server),
      systemMetrics_(sources.feeds.systemMetrics),
      config_(sources.feeds.config),
      recentLogView_(sources.recentLogView) {}

void WebPageHandlers::handleNotFound() {
    server_.send(404, "text/plain", "Not found");
}

void WebPageHandlers::handleFavicon() {
    // No icon shipped — respond empty instead of letting every browser visit
    // burn a 404.
    server_.send(204);
}

// ---------------------------------------------------------------------------
// Chunked streaming helpers
// ---------------------------------------------------------------------------

void WebPageHandlers::sendNavLink(const char* href, const char* label, bool active) {
    char buf[96];
    snprintf(buf, sizeof(buf), "<a href='%s'%s>%s</a>", href, active ? " class='active'" : "",
             label);
    server_.sendContent(buf);
}

void WebPageHandlers::sendHtmlBegin(const char* title, NavPage activePage) {
    // Tell the client we will stream the body — no Content-Length needed.
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");  // open the response
    server_.sendContent(WebAssets::kHtmlHead1);
    server_.sendContent(title);
    server_.sendContent(WebAssets::kHtmlHead2);
    sendNavLink("/", "Home", activePage == NavPage::kHome);
    sendNavLink("/app-info", "App Info", activePage == NavPage::kAppInfo);
    sendNavLink("/system-info", "System Info", activePage == NavPage::kSystemInfo);
    sendNavLink("/logs", "Logs", activePage == NavPage::kLogs);
    sendNavLink("/config", "Config", activePage == NavPage::kConfig);
    sendNavLink("/api", "API", activePage == NavPage::kApi);
    server_.sendContent(WebAssets::kHtmlHead3);
}

void WebPageHandlers::sendHtmlEnd() {
    server_.sendContent(WebAssets::kHtmlFoot);
    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// Dashboard — single dark-themed page served whole from flash (send via
// chunked sendContent, zero per-request heap for the shell itself). All
// dynamic values are pulled client-side from GET /api/status on a 2 s timer,
// so this literal never needs to change per request. Markup lives in
// WebAssets.h alongside the other page-shell blobs.
// ---------------------------------------------------------------------------

void WebPageHandlers::handleHome() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-cache");
    server_.send(200, "text/html", "");
    server_.sendContent(WebAssets::kDashboardHtml);
    server_.sendContent("");  // flush / end chunked transfer
}

void WebPageHandlers::sendSystemInfoBody() {
    char buf[128];
    server_.sendContent("<pre>");
    snprintf(buf, sizeof(buf), "CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "PSRAM Size: %u bytes\n", ESP.getPsramSize());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "PSRAM Free: %u bytes\n", ESP.getFreePsram());
    server_.sendContent(buf);
    snprintf(buf, sizeof(buf), "SDK Version: %s\n", ESP.getSdkVersion());
    server_.sendContent(buf);
    server_.sendContent("</pre>");
}

void WebPageHandlers::handleSystemInfo() {
    sendHtmlBegin("System Information", NavPage::kSystemInfo);
    sendSystemInfoBody();
    sendHtmlEnd();
}

void WebPageHandlers::sendAppInfoBody() {
    char buf[128];

    server_.sendContent("<pre>");

    char uptime[20];
    systemMetrics_.getFormattedUptime(uptime, sizeof(uptime));
    snprintf(buf, sizeof(buf), "Uptime: %s\n", uptime);
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Free Heap: %u bytes\n", ESP.getFreeHeap());
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "NerdWinSense JSON Parse Time: %u ms\n",
             systemMetrics_.getPcMetricsJsonParseTime());
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Screen Draw Time: avg %.2f ms, p95 %.2f ms, max %.2f ms\n",
             systemMetrics_.getAverageScreenDrawTimeUs() / 1000.0f,
             systemMetrics_.getP95ScreenDrawTimeUs() / 1000.0f,
             systemMetrics_.getMaxScreenDrawTimeUs() / 1000.0f);
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Thread Widget FPS: %.1f\n", systemMetrics_.getThreadWidgetFPS());
    server_.sendContent(buf);

    server_.sendContent("</pre>");

    // Draw-times table — streamed row by row, no giant String.
    server_.sendContent(
        "<table class='draw-times'>"
        "<tr><th>Draw</th><th>Draw time (ms)</th></tr>");

    const auto& drawTimesUs = systemMetrics_.getScreenDrawTimesUs();
    const size_t count = systemMetrics_.getScreenDrawCount();
    const size_t start = systemMetrics_.getScreenDrawStartIndex();

    // Walk the circular buffer starting at the oldest sample so "Draw N" is
    // chronological, not raw slot order.
    for (size_t i = 0; i < count && i < drawTimesUs.size(); ++i) {
        snprintf(buf, sizeof(buf), "<tr><td>%u</td><td>%.2f</td></tr>",
                 static_cast<unsigned int>(i + 1),
                 drawTimesUs[(start + i) % drawTimesUs.size()] / 1000.0f);
        server_.sendContent(buf);
    }

    server_.sendContent("</table>");
}

void WebPageHandlers::handleAppInfo() {
    sendHtmlBegin("App Information", NavPage::kAppInfo);
    sendAppInfoBody();
    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /api — help page listing every API endpoint this web server provides.
// ---------------------------------------------------------------------------

namespace {
struct ApiEndpoint {
    const char* method;
    const char* path;
    const char* description;
};

constexpr ApiEndpoint kApiEndpoints[] = {
    {"GET",  "/api/status",
     "Compact JSON snapshot: system health, app metrics, network, PC-metrics feed "
     "status, current screen, task stack high-water marks. Cache-Control: no-store. "
     "app.threads_bar_draw_us is 0 on release builds by design (DEBUG_MODE-gated). "
     "app.widgets is the current screen's top widgets by cumulative draw time "
     "(resets on a screen change); app.noop_dirty_frames counts frames where a "
     "widget was found dirty but nothing was actually drawn. wifi.ssid/rssi/ip/etc. "
     "are always live; wifi.scan_* (nearby-network scan results) only advance while "
     "the WIFI screen is open — the scan job is screen-gated."                          },
    {"GET",  "/api/raw",
     "Last raw NerdWinSense payload, unfiltered — for debugging a field that reads "
     "0 or -1. Triggers a dedicated fetch; not the same connection as the regular "
     "poll. Synchronous: while NerdWinSense answers, every other request (POST /audio "
     "included) waits, up to ~3 s (LAN connect + response timeouts) if it's down."      },
    {"GET",  "/api/pc",
     "Full current PcMetrics snapshot as JSON — every parsed field, including disk "
     "drives. Verify what the display should show vs. what NerdWinSense sent."          },
    {"GET",  "/metrics",
     "Prometheus text-exposition of the same data as /api/status and /api/pc — scrape "
     "target for Prometheus/Grafana, or a bonus sensor source for Home Assistant."      },
    {"POST", "/restart",          "Reboots the device (ESP.restart()). No confirmation."},
    {"POST", "/screen/main",      "Switches the display to the Main screen."            },
    {"POST", "/screen/settings",  "Switches the display to the Settings screen."        },
    {"POST", "/screen/game",      "Switches the display to the Game screen."            },
    {"POST", "/screen/weather",   "Switches the display to the Weather screen."         },
    {"POST", "/screen/calendar",  "Switches the display to the Calendar screen."        },
    {"POST", "/screen/disks",     "Switches the display to the Disks screen."           },
    {"POST", "/screen/cpu-clock", "Switches the display to the CPU Clock screen."       },
    {"POST", "/screen/processes", "Switches the display to the Processes screen."       },
    {"POST", "/screen/wifi",      "Switches the display to the WiFi screen."            },
};

// Streams `text` with HTML special characters escaped, through a small
// stack buffer — no heap String. Log messages can carry text from outside
// the device (a proxy's HTML error page via JsonHttpService, or the `event`
// field of any LAN client's POST /audio), so they must never be written into
// the page raw (B56).
void sendHtmlEscaped(WebServer& server, const char* text) {
    char out[128];
    size_t n = 0;
    auto flush = [&]() {
        out[n] = '\0';
        server.sendContent(out);
        n = 0;
    };
    for (const char* p = text; *p != '\0'; ++p) {
        const char* rep = nullptr;
        switch (*p) {
            case '<':
                rep = "&lt;";
                break;
            case '>':
                rep = "&gt;";
                break;
            case '&':
                rep = "&amp;";
                break;
            case '"':
                rep = "&quot;";
                break;
            case '\'':
                rep = "&#39;";
                break;
            default:
                break;
        }
        const size_t need = rep ? strlen(rep) : 1;
        if (n + need >= sizeof(out)) {
            flush();
        }
        if (rep) {
            memcpy(out + n, rep, need);
            n += need;
        } else {
            out[n++] = *p;
        }
    }
    if (n > 0) {
        flush();
    }
}

}  // namespace

void WebPageHandlers::handleApiHelp() {
    sendHtmlBegin("API", NavPage::kApi);

    server_.sendContent(
        "<p>Endpoints provided by this web server. JSON endpoints are safe to poll "
        "from scripts (curl, Home Assistant, etc.); POST endpoints change device state.</p>"
        "<table><tr><th>Method</th><th>Path</th><th>Description</th></tr>");

    for (const auto& endpoint : kApiEndpoints) {
        // Only GET endpoints are linked — a POST route (/restart, /screen/*)
        // would just 404 on a plain click since it can't send the method.
        char pathCell[192];
        if (strcmp(endpoint.method, "GET") == 0) {
            snprintf(pathCell, sizeof(pathCell), "<a href='%s' target='_blank'><code>%s</code></a>",
                     endpoint.path, endpoint.path);
        } else {
            snprintf(pathCell, sizeof(pathCell), "<code>%s</code>", endpoint.path);
        }
        // Sent piecewise rather than formatted into one fixed buffer: the
        // /api/status description alone is ~590 bytes, and a 384-byte row
        // buffer cut it mid-sentence and dropped the closing tags (B57).
        server_.sendContent("<tr><td>");
        server_.sendContent(endpoint.method);
        server_.sendContent("</td><td>");
        server_.sendContent(pathCell);
        server_.sendContent("</td><td>");
        server_.sendContent(endpoint.description);
        server_.sendContent("</td></tr>");
    }
    server_.sendContent("</table>");

    server_.sendContent(
        "<p>Human-oriented pages: <code>/</code>, <code>/system-info</code>, "
        "<code>/app-info</code>, <code>/config</code> (active tuning constants), "
        "<code>/logs</code> (recent log entries).</p>");

    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /config — active AppSettings values, so a flashed device can be confirmed
// to be running the tuning constants (and debug/release build) expected.
// ---------------------------------------------------------------------------

void WebPageHandlers::handleConfig() {
    sendHtmlBegin("Config", NavPage::kConfig);
    char buf[128];

    server_.sendContent("<pre>");

#define SEND_CONFIG_U(name, value)                                                          \
    do {                                                                                    \
        snprintf(buf, sizeof(buf), "%-32s %lu\n", name, static_cast<unsigned long>(value)); \
        server_.sendContent(buf);                                                           \
    } while (0)
#define SEND_CONFIG_F(name, value)                                 \
    do {                                                           \
        snprintf(buf, sizeof(buf), "%-32s %.3f\n", name, (value)); \
        server_.sendContent(buf);                                  \
    } while (0)
#define SEND_CONFIG_B(name, value)                                                  \
    do {                                                                            \
        snprintf(buf, sizeof(buf), "%-32s %s\n", name, (value) ? "true" : "false"); \
        server_.sendContent(buf);                                                   \
    } while (0)
#define SEND_CONFIG_S(name, value)                               \
    do {                                                         \
        snprintf(buf, sizeof(buf), "%-32s %s\n", name, (value)); \
        server_.sendContent(buf);                                \
    } while (0)

#define AS_FIELD(kind, type, name, init) SEND_CONFIG_##kind(#name, config_.name);
#include "config/AppSettingsFields.def"
#undef AS_FIELD

    {
        char levels[64];
        size_t offset = 0;
        for (uint8_t i = 0; i < AppConfig::Limits::kBrightnessLevelCount; ++i) {
            offset += snprintf(levels + offset, sizeof(levels) - offset, "%s%u", i == 0 ? "" : ",",
                               config_.uiBrightnessLevels[i]);
        }
        snprintf(buf, sizeof(buf), "%-32s %s\n", "uiBrightnessLevels", levels);
        server_.sendContent(buf);
    }

    // The three SseStreamTuning instances — one struct, so one loop.
    {
        struct NamedTuning {
            const char* name;
            const SseStreamTuning& t;
        };
        const NamedTuning tunings[] = {
            {"pcMetricsStream", config_.pcMetricsStream},
            {"cpuClockStream",  config_.cpuClockStream },
            {"processStream",   config_.processStream  }
        };
        char key[48];
        for (const NamedTuning& nt : tunings) {
#define SEND_TUNING(kind, field)                              \
    do {                                                      \
        snprintf(key, sizeof(key), "%s.%s", nt.name, #field); \
        SEND_CONFIG_##kind(key, nt.t.field);                  \
    } while (0)
            SEND_TUNING(U, intervalMs);
            SEND_TUNING(U, connectTimeoutMs);
            SEND_TUNING(U, headerTimeoutMs);
            SEND_TUNING(U, reconnectBackoffMs);
            SEND_TUNING(U, staleTimeoutMs);
            SEND_TUNING(U, maxEventBufferBytes);
            SEND_TUNING(U, maxBytesPerPoll);
            SEND_TUNING(S, streamPath);
#undef SEND_TUNING
        }
    }

    // Endpoints (A9) — only the non-secret ones. wifiSsid/wifiPassword and
    // airVisualApi (its URL carries the API key) are deliberately left out.
    SEND_CONFIG_S("endpoints.pcMetricsApi", config_.endpoints.pcMetricsApi);
    SEND_CONFIG_S("endpoints.roomClimateApi", config_.endpoints.roomClimateApi);
    SEND_CONFIG_S("endpoints.weatherApi", config_.endpoints.weatherApi);
    SEND_CONFIG_F("endpoints.ethDownloadMbps", config_.endpoints.ethDownloadMbps);
    SEND_CONFIG_F("endpoints.ethUploadMbps", config_.endpoints.ethUploadMbps);

#undef SEND_CONFIG_U
#undef SEND_CONFIG_F
#undef SEND_CONFIG_B
#undef SEND_CONFIG_S

    snprintf(buf, sizeof(buf), "%-32s %s\n", "buildMode",
#if DEBUG_MODE
             "debug"
#else
             "release"
#endif
    );
    server_.sendContent(buf);

    server_.sendContent("</pre>");
    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /logs — recent log entries (level >= INFO), non-destructive.
// ---------------------------------------------------------------------------

const char* WebPageHandlers::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

void WebPageHandlers::handleLogs() {
    sendHtmlBegin("Logs", NavPage::kLogs);

    // Heap-allocated — kRecentLogCapacity * sizeof(LogEntry) is too big for a
    // comfortable stack frame on the Arduino loop task.
    auto entries = std::make_unique<LogEntry[]>(RecentLogView::kRecentLogCapacity);
    const size_t count =
        recentLogView_.copyRecentLogs(entries.get(), RecentLogView::kRecentLogCapacity);

    char row[256];
    if (count == 0) {
        server_.sendContent("<p>No log entries recorded yet.</p>");
    } else {
        server_.sendContent("<table><tr><th>Time</th><th>Level</th><th>Message</th></tr>");
        for (size_t i = 0; i < count; ++i) {
            const LogEntry& entry = entries[i];
            snprintf(row, sizeof(row), "<tr><td>%s</td><td>%s</td><td>", entry.timestamp,
                     logLevelToString(entry.level));
            server_.sendContent(row);
            sendHtmlEscaped(server_, entry.message);
            server_.sendContent("</td></tr>");
        }
        server_.sendContent("</table>");
    }

    sendHtmlEnd();
}
