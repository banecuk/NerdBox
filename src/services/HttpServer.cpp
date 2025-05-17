#include "HttpServer.h"

HttpServer::HttpServer(UIController &uiController, ApplicationMetrics &systemMetrics)
    : server_(80), uiController_(uiController), systemMetrics_(systemMetrics) {}

void HttpServer::begin() {
    server_.on("/", [this]() { this->handleHome(); });
    server_.on("/system-info", [this]() { this->handleSystemInfo(); });
    server_.on("/app-info", [this]() { this->handleAppInfo(); });
    server_.on("/screen/main",
               [this]() { uiController_.requestScreen(ScreenName::MAIN); });
    server_.on("/screen/settings",
               [this]() { uiController_.requestScreen(ScreenName::SETTINGS); });
    server_.onNotFound([this]() { this->handleNotFound(); });
    server_.begin();
}

void HttpServer::processRequests() { server_.handleClient(); }

void HttpServer::handleNotFound() { server_.send(404, "text/plain", "Not found"); }

void HttpServer::handleHome() {
    server_.send(200, "text/html", wrapHtmlContent("Homepage", ""));
}

String HttpServer::getSystemInfo() {
    char buffer[300];
    size_t offset = 0;

    // Write opening tag
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<pre>");

    // Free Heap
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Free Heap: %u bytes\n",
                       ESP.getFreeHeap());

    // PSRAM Size
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "PSRAM Size: %u bytes\n",
                       ESP.getPsramSize());

    // PSRAM Free
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "PSRAM Free: %u bytes\n",
                       ESP.getFreePsram());

    // CPU Frequency
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());

    // SDK Version
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "SDK Version: %s\n",
                       ESP.getSdkVersion());

    // Write closing tag
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "</pre>");

    // Convert to String for compatibility
    String info;
    info.reserve(offset + 1);
    info.concat(buffer, offset);

    return wrapHtmlContent("System Information", info);
}

String HttpServer::getAppInfo() {
    char buffer[2048];
    size_t offset = 0;

    // Write metrics in pre tag
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "<pre>");
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "PC Metrics JSON Parse Time: %u ms\n",
                       systemMetrics_.getPcMetricsJsonParseTime());
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "Average Screen Draw Time: %u ms\n",
                       static_cast<uint32_t>(systemMetrics_.getAverageScreenDrawTime()));
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "</pre>");

    // Write screen draw times as a table
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                       "<table class='draw-times'>"
                       "<tr><th>Draw</th><th>Draw time (ms)</th></tr>");
    const auto& drawTimes = systemMetrics_.getScreenDrawTimes();
    size_t count = systemMetrics_.getScreenDrawCount();
    for (size_t i = 0; i < count && i < drawTimes.size(); ++i) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "<tr><td>%u</td><td>%u</td></tr>",
                           i + 1, drawTimes[i]);
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "</table>");

    // Convert to String for compatibility
    String info;
    info.reserve(offset + 1);
    info.concat(buffer, offset);

    return wrapHtmlContent("App Information", info);
}

void HttpServer::handleSystemInfo() { server_.send(200, "text/html", getSystemInfo()); }

void HttpServer::handleAppInfo() { server_.send(200, "text/html", getAppInfo()); }

String HttpServer::wrapHtmlContent(const String &title, const String &content) {
    // Static HTML parts stored in flash
    static constexpr char kHtmlPrefix[] =
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>";
    static constexpr char kTitleSuffix[] =
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
    static constexpr char kHeaderSuffix[] =
        "</h1>"
        "<nav>"
        "<ul>"
        "<li><a href='/'>Home</a></li>"
        "<li><a href='/app-info'>App Info</a></li>"
        "<li><a href='/system-info'>System Info</a></li>"
        "</ul>"
        "</nav>"
        "</div>"
        "</header>"
        "<div class='content'>";
    static constexpr char kHtmlSuffix[] =
        "</div>"
        "<footer>NerdBox 2025<br /><small>WT32-SC01-PLUS</small></footer>"
        "</body></html>";

    // Estimate required size to minimize reallocations
    size_t estimatedSize = sizeof(kHtmlPrefix) + title.length() + sizeof(kTitleSuffix) +
                           sizeof(kHeaderSuffix) + content.length() + sizeof(kHtmlSuffix);
    String html;
    html.reserve(estimatedSize);

    // Append static and dynamic parts
    html.concat(kHtmlPrefix, sizeof(kHtmlPrefix) - 1);
    html.concat(title.c_str(), title.length());
    html.concat(kTitleSuffix, sizeof(kTitleSuffix) - 1);
    html.concat(kHeaderSuffix, sizeof(kHeaderSuffix) - 1);
    html.concat(content.c_str(), content.length());
    html.concat(kHtmlSuffix, sizeof(kHtmlSuffix) - 1);

    return html;
}