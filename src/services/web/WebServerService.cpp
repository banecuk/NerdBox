#include "WebServerService.h"

#include "core/events/EventBus.h"

namespace {
struct ScreenRoute {
    const char* path;
    ScreenName screen;
};

// One entry per POST /screen/<name> route — see begin()'s registration loop.
constexpr ScreenRoute kScreenRoutes[] = {
    {"/screen/main",       ScreenName::MAIN     },
    {"/screen/settings",   ScreenName::SETTINGS },
    {"/screen/game",       ScreenName::GAME     },
    {"/screen/weather",    ScreenName::WEATHER  },
    {"/screen/calendar",   ScreenName::CALENDAR },
    {"/screen/disks",      ScreenName::DISKS    },
    {"/screen/cpu-clock",  ScreenName::CPU_CLOCK},
    {"/screen/processes",  ScreenName::PROCESSES},
};
}  // namespace

WebServerService::WebServerService(
    WebServer& server, IScreenNavigator& screenNavigator, ApplicationMetrics& systemMetrics,
    PcMetrics& pcMetrics, PcMetricsService& pcMetricsService, IStreamHealth& pcMetricsStreamJob,
    IStreamHealth& cpuClockStreamJob, IStreamHealth& processStreamJob,
    const NetworkStatus& netStatus, const SystemState& systemState, const WeatherData& weatherData,
    const AppSettings& config, const ITaskStackReporter& taskStackReporter,
    LoggerInterface& logger, RecentLogView& recentLogView, const AudioData& audioData,
    AudioService& audioService, const RoomClimateData& roomClimateData)
    : server_(server),
      screenNavigator_(screenNavigator),
      logger_(logger),
      audioService_(audioService),
      apiHandlers_(server, systemMetrics, pcMetrics, pcMetricsService, pcMetricsStreamJob,
                   cpuClockStreamJob, processStreamJob, netStatus, systemState, weatherData,
                   config, taskStackReporter, audioData, roomClimateData),
      pageHandlers_(server, systemMetrics, config, recentLogView) {}

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
    for (const auto& route : kScreenRoutes) {
        const ScreenName screen = route.screen;
        server_.on(route.path, HTTP_POST, [this, screen]() {
            screenNavigator_.requestScreen(screen);
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
    logger_.info("Restart requested via web server");
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
    const bool needsResync = audioService_.handlePush(body);
    // The plugin only inspects HTTP 409 or a body containing "resend" — any
    // other response is ordinary success/failure and otherwise ignored.
    if (needsResync) {
        server_.send(409, "text/plain", "resend");
    } else {
        server_.send(200, "text/plain", "OK");
    }
}
