#pragma once

#include <WebServer.h>

#include "config/AppSettings.h"
#include "utils/ApplicationMetrics.h"
#include "utils/RecentLogView.h"

// The human-facing HTML pages (dashboard, /system-info, /app-info, /config,
// /logs, /api help), split out of WebServerService so that class isn't also
// responsible for HTML shell rendering. Everything is streamed directly to
// the client in chunks via WebServer::sendContent — no large String
// assembled on the heap.
class WebPageHandlers {
 public:
    WebPageHandlers(WebServer& server, ApplicationMetrics& systemMetrics, const AppSettings& config,
                    RecentLogView& recentLogView);

    void handleNotFound();
    void handleHome();
    void handleFavicon();
    void handleSystemInfo();
    void handleAppInfo();
    void handleApiHelp();
    void handleConfig();
    void handleLogs();

 private:
    // Identifies which nav-bar link to highlight as active in sendHtmlBegin.
    // Mirrors the pages listed in WebAssets::kHtmlHead2/kHtmlHead3's nav.
    enum class NavPage {
        kHome,
        kAppInfo,
        kSystemInfo,
        kLogs,
        kConfig,
        kApi
    };

    // Streams the HTML wrapper and body content directly to the client in
    // chunks — no large String assembled on the heap.
    void sendHtmlBegin(const char* title, NavPage activePage);
    void sendNavLink(const char* href, const char* label, bool active);
    void sendHtmlEnd();
    void sendSystemInfoBody();
    void sendAppInfoBody();

    static const char* logLevelToString(LogLevel level);

    WebServer& server_;
    ApplicationMetrics& systemMetrics_;
    const AppSettings& config_;
    RecentLogView& recentLogView_;
};
