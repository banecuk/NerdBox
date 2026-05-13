#include "WebServerService.h"

WebServerService::WebServerService(WebServer& server, UiController& uiController,
                                   ApplicationMetrics& systemMetrics)
    : server_(server), uiController_(uiController), systemMetrics_(systemMetrics) {}

void WebServerService::begin() {
    server_.on("/", [this]() { this->handleHome(); });
    server_.on("/system-info", [this]() { this->handleSystemInfo(); });
    server_.on("/app-info", [this]() { this->handleAppInfo(); });
    server_.on("/screen/main", [this]() { uiController_.requestScreen(ScreenName::MAIN); });
    server_.on("/screen/settings", [this]() { uiController_.requestScreen(ScreenName::SETTINGS); });
    server_.onNotFound([this]() { this->handleNotFound(); });
    server_.begin();
}

void WebServerService::processRequests() {
    server_.handleClient();
}

void WebServerService::handleNotFound() {
    server_.send(404, "text/plain", "Not found");
}

// ---------------------------------------------------------------------------
// Chunked streaming helpers
// ---------------------------------------------------------------------------

// HTML is split into two static flash-resident blocks so the title can be
// injected in the middle without building a temporary String.
static constexpr char kHtmlHead1[] =
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>";

static constexpr char kHtmlHead2[] =
    " - NerdBox</title>"
    "<style>"
    "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; "
    "line-height: 1.6; color: #333; }"
    "header { background: #2c3e50; color: white; padding: 1rem 0; margin-bottom: "
    "1.5rem; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"
    ".header-content { max-width: 1200px; margin: 0 auto; padding: 0 1rem; }"
    "nav { margin-top: 1rem; }"
    "nav ul { list-style: none; padding: 0; margin: 0; display: flex; gap: 1rem; }"
    "nav a { color: white; text-decoration: none; padding: 0.5rem 1rem; "
    "border-radius: 4px; transition: background-color 0.3s; }"
    "nav a:hover { background-color: #34495e; }"
    "h1 { margin: 0; }"
    ".content { max-width: 1200px; margin: 0 auto; padding: 0 1rem; }"
    "footer { background: #f8f9fa; margin-top: 2rem; padding: 1.5rem 0; border-top: "
    "1px solid #ddd; color: #666; text-align: center; }"
    "pre { background: #f8f9fa; padding: 1.5rem; border-radius: 5px; overflow-x: "
    "auto; border: 1px solid #ddd; }"
    "</style></head>"
    "<body>"
    "<header>"
    "<div class='header-content'>"
    "<span>NerdBox</span>"
    "<h1>";

static constexpr char kHtmlHead3[] =
    "</h1>"
    "<nav><ul>"
    "<li><a href='/'>Home</a></li>"
    "<li><a href='/app-info'>App Info</a></li>"
    "<li><a href='/system-info'>System Info</a></li>"
    "</ul></nav>"
    "</div></header>"
    "<div class='content'>";

static constexpr char kHtmlFoot[] =
    "</div>"
    "<footer>NerdBox 2025<br /><small>WT32-SC01-PLUS</small></footer>"
    "</body></html>";

void WebServerService::sendHtmlBegin(const char* title) {
    // Tell the client we will stream the body — no Content-Length needed.
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");      // open the response
    server_.sendContent(kHtmlHead1);
    server_.sendContent(title);
    server_.sendContent(kHtmlHead2);
    server_.sendContent(title);              // repeated in <h1>
    server_.sendContent(kHtmlHead3);
}

void WebServerService::sendHtmlEnd() {
    server_.sendContent(kHtmlFoot);
    server_.sendContent("");                 // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

void WebServerService::handleHome() {
    sendHtmlBegin("Homepage");
    sendHtmlEnd();
}

void WebServerService::sendSystemInfoBody() {
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

void WebServerService::handleSystemInfo() {
    sendHtmlBegin("System Information");
    sendSystemInfoBody();
    sendHtmlEnd();
}

void WebServerService::sendAppInfoBody() {
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

    snprintf(buf, sizeof(buf), "Average Screen Draw Time: %u ms\n",
             static_cast<uint32_t>(systemMetrics_.getAverageScreenDrawTime()));
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "Thread Widget FPS: %.1f\n", systemMetrics_.getThreadWidgetFPS());
    server_.sendContent(buf);

    server_.sendContent("</pre>");

    // Draw-times table — streamed row by row, no giant String.
    server_.sendContent(
        "<table class='draw-times'>"
        "<tr><th>Draw</th><th>Draw time (ms)</th></tr>");

    const auto& drawTimes = systemMetrics_.getScreenDrawTimes();
    const size_t count    = systemMetrics_.getScreenDrawCount();

    for (size_t i = 0; i < count && i < drawTimes.size(); ++i) {
        snprintf(buf, sizeof(buf), "<tr><td>%u</td><td>%u</td></tr>",
                 static_cast<unsigned int>(i + 1), drawTimes[i]);
        server_.sendContent(buf);
    }

    server_.sendContent("</table>");
}

void WebServerService::handleAppInfo() {
    sendHtmlBegin("App Information");
    sendAppInfoBody();
    sendHtmlEnd();
}
