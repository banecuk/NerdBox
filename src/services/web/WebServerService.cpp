#include "WebServerService.h"

#include "core/events/EventBus.h"
#include "core/IScreenNavigator.h"
#include "core/ScreenRegistry.h"
#include "services/audio/AudioService.h"
#include "utils/logging/LoggerInterface.h"

WebServerService::WebServerService(WebServer& server, const WebDataSources& sources)
    : server_(server),
      sources_(sources),
      apiHandlers_(server, sources_),
      pageHandlers_(server, sources_) {}

void WebServerService::begin() {
    server_.on("/", [this]() { pageHandlers_.handleHome(); });
    server_.on("/system-info", [this]() { pageHandlers_.handleSystemInfo(); });
    server_.on("/app-info", [this]() { pageHandlers_.handleAppInfo(); });
    server_.on("/api", [this]() { pageHandlers_.handleApiHelp(); });
    server_.on("/api/status", [this]() { apiHandlers_.handleApiStatus(); });
    server_.on("/api/raw", [this]() { apiHandlers_.handleApiRaw(); });
    server_.on("/api/pc", [this]() { apiHandlers_.handleApiPc(); });
    server_.on("/metrics", [this]() { apiHandlers_.handleMetrics(); });
    server_.on("/config", [this]() { pageHandlers_.handleConfig(); });
    server_.on("/logs", [this]() { pageHandlers_.handleLogs(); });
    server_.on("/restart", HTTP_POST, [this]() { this->handleRestart(); });
    // mb_NerdBox MusicBee plugin push target — default PushUrl path, see
    // docs-local/NERDBOX_INTEGRATION.md.
    server_.on("/audio", HTTP_POST, [this]() { this->handleAudioPush(); });
    server_.on("/favicon.ico", [this]() { pageHandlers_.handleFavicon(); });
    // POST-only: these change device state, so a GET (e.g. a LAN prefetcher
    // or a link crawler) can no longer flip the screen as a side effect.
    for (const auto& descriptor : kScreens) {
        if (descriptor.route == nullptr)
            continue;
        const ScreenName screen = descriptor.screen;
        server_.on(descriptor.route, HTTP_POST, [this, screen]() {
            sources_.screenNavigator.requestScreen(screen);
            server_.send(200, "text/plain", "OK");
        });
    }
    server_.onNotFound([this]() { pageHandlers_.handleNotFound(); });
    server_.begin();
}

void WebServerService::processRequests() {
    server_.handleClient();
}

// ---------------------------------------------------------------------------
// /restart — remote recovery without a power cycle.
// ---------------------------------------------------------------------------

void WebServerService::handleRestart() {
    sources_.logger.info("Restart requested via web server");
    server_.send(200, "text/plain", "Restarting...");
    server_.client().flush();
    delay(200);  // give the TCP stack time to push the response before reboot
    EventBus::getInstance().publish(EventType::RESET_DEVICE);
}

// ---------------------------------------------------------------------------
// POST /audio — mb_NerdBox MusicBee plugin push events.
// ---------------------------------------------------------------------------

void WebServerService::handleAudioPush() {
    const String body = server_.arg("plain");
    const bool needsResync =
        sources_.audioService.handlePush(body.c_str(), body.length(), millis());
    // The plugin only inspects HTTP 409 or a body containing "resend" — any
    // other response is ordinary success/failure and otherwise ignored.
    if (needsResync) {
        server_.send(409, "text/plain", "resend");
    } else {
        server_.send(200, "text/plain", "OK");
    }
}
