#include "WebServerService.h"

#include <WiFi.h>
#include <esp_system.h>

#include "core/events/EventBus.h"
#include "utils/DataFreshnessGuard.h"

WebServerService::WebServerService(WebServer& server, UiController& uiController,
                                   ApplicationMetrics& systemMetrics, PcMetrics& pcMetrics,
                                   PcMetricsService& pcMetricsService,
                                   const NetworkStatus& netStatus, const SystemState& systemState,
                                   const AppSettings& config, const TaskManager& taskManager,
                                   LoggerInterface& logger)
    : server_(server),
      uiController_(uiController),
      systemMetrics_(systemMetrics),
      pcMetrics_(pcMetrics),
      pcMetricsService_(pcMetricsService),
      netStatus_(netStatus),
      systemState_(systemState),
      config_(config),
      taskManager_(taskManager),
      logger_(logger) {}

void WebServerService::begin() {
    server_.on("/", [this]() { this->handleHome(); });
    server_.on("/system-info", [this]() { this->handleSystemInfo(); });
    server_.on("/app-info", [this]() { this->handleAppInfo(); });
    server_.on("/api", [this]() { this->handleApiHelp(); });
    server_.on("/api/status", [this]() { this->handleApiStatus(); });
    server_.on("/api/raw", [this]() { this->handleApiRaw(); });
    server_.on("/api/pc", [this]() { this->handleApiPc(); });
    server_.on("/config", [this]() { this->handleConfig(); });
    server_.on("/logs", [this]() { this->handleLogs(); });
    server_.on("/restart", HTTP_POST, [this]() { this->handleRestart(); });
    server_.on("/favicon.ico", [this]() { this->handleFavicon(); });
    // POST-only: these change device state, so a GET (e.g. a LAN prefetcher
    // or a link crawler) can no longer flip the screen as a side effect.
    server_.on("/screen/main", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::MAIN);
        server_.send(200, "text/plain", "OK");
    });
    server_.on("/screen/settings", HTTP_POST, [this]() {
        uiController_.requestScreen(ScreenName::SETTINGS);
        server_.send(200, "text/plain", "OK");
    });
    server_.onNotFound([this]() { this->handleNotFound(); });
    server_.begin();
}

void WebServerService::processRequests() {
    server_.handleClient();
}

void WebServerService::handleNotFound() {
    server_.send(404, "text/plain", "Not found");
}

void WebServerService::handleFavicon() {
    // No icon shipped — respond empty instead of letting every browser visit
    // burn a 404.
    server_.send(204);
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
    ":root{--bg:#0f1419;--card:#1a2129;--line:#2a3441;--fg:#d8e0ea;--dim:#7a8794;--accent:#4fc3f7;}"
    "*{box-sizing:border-box;}"
    "body{background:var(--bg);color:var(--fg);font-family:'Segoe UI',Arial,sans-serif;"
    "margin:0;padding:0;line-height:1.6;}"
    ".bar{max-width:1200px;margin:0 auto;box-sizing:border-box;}"
    "header{background:var(--card);border-bottom:1px solid var(--line);}"
    "header .bar{padding:0.9rem 1.2rem;display:flex;align-items:baseline;"
    "justify-content:space-between;flex-wrap:wrap;gap:0.5rem;}"
    "header h1{margin:0;font-size:1.3rem;color:var(--accent);}"
    "header .meta{color:var(--dim);font-size:0.85rem;}"
    "nav{background:var(--card);border-bottom:1px solid var(--line);}"
    "nav .bar{padding:0.6rem 1.2rem;display:flex;gap:1rem;font-size:0.85rem;flex-wrap:wrap;}"
    "nav a{color:var(--dim);text-decoration:none;}"
    "nav a:hover{color:var(--accent);}"
    ".content{max-width:1200px;margin:0 auto;padding:1.2rem;}"
    "pre{background:var(--card);padding:1rem;border-radius:8px;overflow-x:auto;"
    "border:1px solid var(--line);font-family:Consolas,monospace;color:var(--fg);}"
    "table{border-collapse:collapse;width:100%;margin-top:0.5rem;font-size:0.9rem;}"
    "th,td{padding:0.4rem 0.6rem;border-bottom:1px solid var(--line);text-align:left;}"
    "th{color:var(--dim);font-weight:600;}"
    "code{background:var(--card);border:1px solid var(--line);border-radius:4px;"
    "padding:0.1rem 0.35rem;}"
    "</style></head>"
    "<body>"
    "<header><div class='bar'>"
    "<h1>NerdBox</h1>"
    "<div class='meta'>";

static constexpr char kHtmlHead3[] =
    "</div>"
    "</div></header>"
    "<nav><div class='bar'>"
    "<a href='/'>Home</a>"
    "<a href='/app-info'>App Info</a>"
    "<a href='/system-info'>System Info</a>"
    "<a href='/logs'>Logs</a>"
    "<a href='/config'>Config</a>"
    "<a href='/api'>API</a>"
    "</div></nav>"
    "<div class='content'>";

static constexpr char kHtmlFoot[] = "</div></body></html>";

void WebServerService::sendHtmlBegin(const char* title) {
    // Tell the client we will stream the body — no Content-Length needed.
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");      // open the response
    server_.sendContent(kHtmlHead1);
    server_.sendContent(title);
    server_.sendContent(kHtmlHead2);
    server_.sendContent(title);              // repeated in the header's meta area
    server_.sendContent(kHtmlHead3);
}

void WebServerService::sendHtmlEnd() {
    server_.sendContent(kHtmlFoot);
    server_.sendContent("");                 // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Dashboard — single dark-themed page served whole from flash (send via
// chunked sendContent, zero per-request heap for the shell itself). All
// dynamic values are pulled client-side from GET /api/status on a 2 s timer,
// so this literal never needs to change per request.
// ---------------------------------------------------------------------------

static constexpr char kDashboardHtml[] =
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>NerdBox</title>"
    "<style>"
    ":root{--bg:#0f1419;--card:#1a2129;--line:#2a3441;--fg:#d8e0ea;--dim:#7a8794;"
    "--accent:#4fc3f7;--ok:#66bb6a;--warn:#ffa726;--bad:#ef5350;}"
    "*{box-sizing:border-box;}"
    "body{background:var(--bg);color:var(--fg);font-family:'Segoe UI',Arial,sans-serif;"
    "margin:0;padding:0;line-height:1.5;transition:opacity .3s;}"
    "body.offline{opacity:.45;}"
    ".bar{max-width:1200px;margin:0 auto;box-sizing:border-box;}"
    "header{background:var(--card);border-bottom:1px solid var(--line);}"
    "header .bar{padding:0.9rem 1.2rem;display:flex;align-items:baseline;"
    "justify-content:space-between;flex-wrap:wrap;gap:0.5rem;}"
    "header h1{margin:0;font-size:1.3rem;color:var(--accent);}"
    "header .meta{color:var(--dim);font-size:0.85rem;}"
    "header .meta b{color:var(--fg);}"
    "nav{background:var(--card);border-bottom:1px solid var(--line);}"
    "nav .bar{padding:0.6rem 1.2rem;display:flex;gap:1rem;font-size:0.85rem;flex-wrap:wrap;}"
    "nav a{color:var(--dim);text-decoration:none;}"
    "nav a:hover{color:var(--accent);}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));"
    "gap:1rem;padding:1.2rem;max-width:1200px;margin:0 auto;}"
    ".card{background:var(--card);border:1px solid var(--line);border-radius:8px;"
    "padding:1rem;}"
    ".card h2{margin:0 0 0.7rem;font-size:0.8rem;text-transform:uppercase;"
    "letter-spacing:0.05em;color:var(--dim);}"
    ".row{display:flex;justify-content:space-between;padding:0.2rem 0;font-size:0.92rem;}"
    ".row .k{color:var(--dim);}"
    ".row .v{font-variant-numeric:tabular-nums;}"
    ".v.ok{color:var(--ok);}.v.warn{color:var(--warn);}.v.bad{color:var(--bad);}"
    "canvas{width:100%;height:60px;margin-top:0.4rem;}"
    ".btns{display:flex;gap:0.6rem;flex-wrap:wrap;}"
    "button{background:var(--bg);color:var(--fg);border:1px solid var(--line);"
    "border-radius:6px;padding:0.5rem 0.9rem;cursor:pointer;font-size:0.85rem;}"
    "button:hover{border-color:var(--accent);color:var(--accent);}"
    "button.danger:hover{border-color:var(--bad);color:var(--bad);}"
    "</style></head><body>"
    "<header><div class='bar'><h1>NerdBox</h1>"
    "<div class='meta'>Screen: <b id='screenName'>-</b> &middot; "
    "Updated <b id='refreshAge'>-</b></div></div></header>"
    "<nav><div class='bar'><a href='/app-info'>App Info</a><a href='/system-info'>System Info</a>"
    "<a href='/logs'>Logs</a><a href='/config'>Config</a><a href='/api'>API</a></div></nav>"
    "<div class='grid'>"

    "<div class='card'><h2>Device</h2>"
    "<div class='row'><span class='k'>Uptime</span><span class='v' id='uptime'>-</span></div>"
    "<div class='row'><span class='k'>Free heap</span><span class='v' id='heapFree'>-</span></div>"
    "<div class='row'><span class='k'>Min free heap</span><span class='v' id='heapMin'>-</span></div>"
    "<div class='row'><span class='k'>Free PSRAM</span><span class='v' id='psramFree'>-</span></div>"
    "<div class='row'><span class='k'>CPU freq</span><span class='v' id='cpuMhz'>-</span></div>"
    "</div>"

    "<div class='card'><h2>Network</h2>"
    "<div class='row'><span class='k'>WiFi</span><span class='v' id='wifi'>-</span></div>"
    "<div class='row'><span class='k'>RSSI</span><span class='v' id='rssi'>-</span></div>"
    "<div class='row'><span class='k'>IP</span><span class='v' id='ip'>-</span></div>"
    "<div class='row'><span class='k'>Internet</span><span class='v' id='internet'>-</span></div>"
    "<div class='row'><span class='k'>Probes</span><span class='v' id='probes'>-</span></div>"
    "</div>"

    "<div class='card'><h2>PC metrics feed</h2>"
    "<div class='row'><span class='k'>Status</span><span class='v' id='pcStatus'>-</span></div>"
    "<div class='row'><span class='k'>Data age</span><span class='v' id='pcAge'>-</span></div>"
    "<div class='row'><span class='k'>Parse time</span><span class='v' id='parseMs'>-</span></div>"
    "<div class='row'><span class='k'>Fetch ok/fail</span>"
    "<span class='v'><span id='fetchOk'>-</span> / <span id='fetchFail'>-</span></span></div>"
    "<div class='row'><span class='k'>Last error</span><span class='v' id='lastError'>-</span></div>"
    "</div>"

    "<div class='card'><h2>Rendering</h2>"
    "<div class='row'><span class='k'>Avg draw time</span><span class='v' id='drawAvg'>-</span></div>"
    "<div class='row'><span class='k'>Widget FPS</span><span class='v' id='fps'>-</span></div>"
    "<canvas id='spark' width='300' height='60'></canvas>"
    "</div>"

    "<div class='card'><h2>Tasks</h2>"
    "<div class='row'><span class='k'>Screen stack free</span><span class='v' id='screenStack'>-</span></div>"
    "<div class='row'><span class='k'>Background stack free</span><span class='v' id='bgStack'>-</span></div>"
    "</div>"

    "<div class='card'><h2>Screen</h2><div class='btns'>"
    "<button id='btnMain'>Main</button>"
    "<button id='btnSettings'>Settings</button>"
    "<button id='btnRestart' class='danger'>Restart</button>"
    "</div></div>"

    "</div>"
    "<script>"
    "const $=id=>document.getElementById(id);"
    "let lastOk=Date.now();"
    "function setLevel(el,level){el.classList.remove('ok','warn','bad');el.classList.add(level);}"
    "function drawSpark(times){"
    "const c=$('spark'),ctx=c.getContext('2d');const w=c.width,h=c.height;"
    "ctx.clearRect(0,0,w,h);if(!times||!times.length)return;"
    "const max=Math.max.apply(null,times.concat([1]));"
    "ctx.strokeStyle='#4fc3f7';ctx.lineWidth=2;ctx.beginPath();"
    "times.forEach(function(t,i){"
    "const x=i/((times.length-1)||1)*w,y=h-(t/max)*h;"
    "if(i===0){ctx.moveTo(x,y);}else{ctx.lineTo(x,y);}"
    "});ctx.stroke();"
    "}"
    "async function refresh(){"
    "try{"
    "const r=await fetch('/api/status',{cache:'no-store'});"
    "const d=await r.json();lastOk=Date.now();document.body.classList.remove('offline');"
    "$('screenName').textContent=d.ui.screen;"
    "$('uptime').textContent=d.app.uptime;"
    "$('heapFree').textContent=d.sys.heap_free.toLocaleString()+' B';"
    "setLevel($('heapFree'),d.sys.heap_free<50000?'bad':d.sys.heap_free<100000?'warn':'ok');"
    "$('heapMin').textContent=d.sys.heap_min.toLocaleString()+' B';"
    "$('psramFree').textContent=d.sys.psram_free.toLocaleString()+' B';"
    "$('cpuMhz').textContent=d.sys.cpu_mhz+' MHz';"
    "$('drawAvg').textContent=d.app.draw_avg_ms.toFixed(1)+' ms';"
    "$('fps').textContent=d.app.widget_fps.toFixed(1);"
    "$('parseMs').textContent=d.app.parse_ms+' ms';"
    "drawSpark(d.app.draw_times);"
    "$('wifi').textContent=d.net.wifi?'Connected':'Disconnected';"
    "setLevel($('wifi'),d.net.wifi?'ok':'bad');"
    "$('rssi').textContent=d.net.rssi+' dBm';"
    "$('ip').textContent=d.net.ip;"
    "$('internet').textContent=d.net.internet;"
    "setLevel($('internet'),d.net.internet==='OK'?'ok':d.net.internet==='DOWN'?'bad':'warn');"
    "$('probes').textContent=d.net.probes.map(function(p){return p?'\\u25cf':'\\u25cb';}).join(' ');"
    "$('pcStatus').textContent=!d.pc.available?'Unavailable':(d.pc.fresh?'Fresh':'Stale');"
    "setLevel($('pcStatus'),!d.pc.available?'bad':(d.pc.fresh?'ok':'warn'));"
    "$('pcAge').textContent=d.pc.age_ms+' ms';"
    "$('fetchOk').textContent=d.pc.fetch_ok;"
    "$('fetchFail').textContent=d.pc.fetch_fail;"
    "$('lastError').textContent=d.pc.last_error&&d.pc.last_error.length?d.pc.last_error:'\\u2014';"
    "$('screenStack').textContent=d.tasks.screen_stack_free;"
    "$('bgStack').textContent=d.tasks.background_stack_free;"
    "}catch(e){document.body.classList.add('offline');}"
    "$('refreshAge').textContent=Math.round((Date.now()-lastOk)/1000)+'s ago';"
    "}"
    "async function post(path){await fetch(path,{method:'POST'});refresh();}"
    "$('btnMain').onclick=function(){post('/screen/main');};"
    "$('btnSettings').onclick=function(){post('/screen/settings');};"
    "$('btnRestart').onclick=function(){if(confirm('Restart device?'))post('/restart');};"
    "refresh();setInterval(refresh,2000);"
    "</script></body></html>";

void WebServerService::handleHome() {
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-cache");
    server_.send(200, "text/html", "");
    server_.sendContent(kDashboardHtml);
    server_.sendContent("");  // flush / end chunked transfer
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
    const size_t start    = systemMetrics_.getScreenDrawStartIndex();

    // Walk the circular buffer starting at the oldest sample so "Draw N" is
    // chronological, not raw slot order.
    for (size_t i = 0; i < count && i < drawTimes.size(); ++i) {
        snprintf(buf, sizeof(buf), "<tr><td>%u</td><td>%u</td></tr>",
                 static_cast<unsigned int>(i + 1), drawTimes[(start + i) % drawTimes.size()]);
        server_.sendContent(buf);
    }

    server_.sendContent("</table>");
}

void WebServerService::handleAppInfo() {
    sendHtmlBegin("App Information");
    sendAppInfoBody();
    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /api/status — JSON snapshot for scriptable monitoring / the dashboard.
// Streamed in small pieces via sendContent — no JsonDocument, no String.
// ---------------------------------------------------------------------------

const char* WebServerService::internetStatusToString(NetworkStatus::Internet status) {
    switch (status) {
        case NetworkStatus::Internet::OK: return "OK";
        case NetworkStatus::Internet::WARNING: return "WARNING";
        case NetworkStatus::Internet::DEGRADED: return "DEGRADED";
        case NetworkStatus::Internet::DOWN: return "DOWN";
        case NetworkStatus::Internet::UNKNOWN:
        default: return "UNKNOWN";
    }
}

const char* WebServerService::screenNameToString(ScreenName screen) {
    switch (screen) {
        case ScreenName::BOOT: return "BOOT";
        case ScreenName::MAIN: return "MAIN";
        case ScreenName::SETTINGS: return "SETTINGS";
        case ScreenName::NONE:
        default: return "NONE";
    }
}

void WebServerService::handleApiStatus() {
    char buf[192];

    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", "");

    snprintf(buf, sizeof(buf),
             "{\"sys\":{\"heap_free\":%u,\"heap_min\":%u,\"heap_max_alloc\":%u,"
             "\"psram_size\":%u,\"psram_free\":%u,\"cpu_mhz\":%u,\"sdk\":\"%s\","
             "\"reset_reason\":%d},",
             ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap(), ESP.getPsramSize(),
             ESP.getFreePsram(), ESP.getCpuFreqMHz(), ESP.getSdkVersion(),
             static_cast<int>(esp_reset_reason()));
    server_.sendContent(buf);

    char uptime[20];
    systemMetrics_.getFormattedUptime(uptime, sizeof(uptime));
    snprintf(buf, sizeof(buf),
             "\"app\":{\"uptime\":\"%s\",\"uptime_ms\":%lu,\"parse_ms\":%u,"
             "\"draw_avg_ms\":%.1f,\"widget_fps\":%.1f,\"draw_times\":[",
             uptime, millis(), systemMetrics_.getPcMetricsJsonParseTime(),
             systemMetrics_.getAverageScreenDrawTime(), systemMetrics_.getThreadWidgetFPS());
    server_.sendContent(buf);

    const auto& drawTimes = systemMetrics_.getScreenDrawTimes();
    const size_t drawCount = systemMetrics_.getScreenDrawCount();
    const size_t drawStart = systemMetrics_.getScreenDrawStartIndex();
    for (size_t i = 0; i < drawCount && i < drawTimes.size(); ++i) {
        snprintf(buf, sizeof(buf), "%s%u", i == 0 ? "" : ",",
                 drawTimes[(drawStart + i) % drawTimes.size()]);
        server_.sendContent(buf);
    }
    server_.sendContent("]},");

    char ip[16] = "0.0.0.0";
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress addr = WiFi.localIP();
        snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    }
    snprintf(buf, sizeof(buf), "\"net\":{\"wifi\":%s,\"rssi\":%d,\"ip\":\"%s\",\"internet\":\"%s\",\"probes\":[",
             netStatus_.wifi_connected ? "true" : "false", netStatus_.rssi, ip,
             internetStatusToString(netStatus_.internet));
    server_.sendContent(buf);

    for (uint8_t i = 0; i < 6; ++i) {
        snprintf(buf, sizeof(buf), "%s%d", i == 0 ? "" : ",", netStatus_.endpoint_ok[i] ? 1 : 0);
        server_.sendContent(buf);
    }
    server_.sendContent("]},");

    const DataFreshnessGuard freshness(pcMetrics_.is_available, pcMetrics_.last_update_timestamp);
    const unsigned long ageMs =
        pcMetrics_.is_available ? millis() - pcMetrics_.last_update_timestamp : 0;
    snprintf(buf, sizeof(buf),
             "\"pc\":{\"available\":%s,\"fresh\":%s,\"age_ms\":%lu,"
             "\"fetch_ok\":%u,\"fetch_fail\":%u,\"last_error\":\"%s\"},",
             pcMetrics_.is_available ? "true" : "false", freshness.isFresh() ? "true" : "false",
             ageMs, pcMetricsService_.getFetchOkCount(), pcMetricsService_.getFetchFailCount(),
             pcMetricsService_.getLastError());
    server_.sendContent(buf);

    snprintf(buf, sizeof(buf), "\"ui\":{\"screen\":\"%s\",\"time_synced\":%s},",
             screenNameToString(systemState_.screen.activeScreen),
             systemState_.core.isTimeSynced ? "true" : "false");
    server_.sendContent(buf);

    // Stack high-water marks — early warning before a tight task stack
    // (6144/8096 B) overflows and reboots the device. 0 means the task
    // hasn't been created yet.
    const TaskHandle_t screenTask = taskManager_.getScreenTaskHandle();
    const TaskHandle_t backgroundTask = taskManager_.getBackgroundTaskHandle();
    snprintf(buf, sizeof(buf), "\"tasks\":{\"screen_stack_free\":%u,\"background_stack_free\":%u}}",
             screenTask ? uxTaskGetStackHighWaterMark(screenTask) : 0,
             backgroundTask ? uxTaskGetStackHighWaterMark(backgroundTask) : 0);
    server_.sendContent(buf);

    server_.sendContent("");  // flush / end chunked transfer
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
// /api — help page listing every API endpoint this web server provides.
// ---------------------------------------------------------------------------

namespace {
struct ApiEndpoint {
    const char* method;
    const char* path;
    const char* description;
};

constexpr ApiEndpoint kApiEndpoints[] = {
    {"GET", "/api/status",
     "Compact JSON snapshot: system health, app metrics, network, PC-metrics feed "
     "status, current screen, task stack high-water marks. Cache-Control: no-store."},
    {"GET", "/api/raw",
     "Last raw NerdWinSense payload, unfiltered — for debugging a field that reads "
     "0 or -1. Triggers a dedicated fetch; not the same connection as the regular "
     "poll."},
    {"GET", "/api/pc",
     "Full current PcMetrics snapshot as JSON — every parsed field, including disk "
     "drives. Verify what the display should show vs. what NerdWinSense sent."},
    {"POST", "/restart", "Reboots the device (ESP.restart()). No confirmation."},
    {"POST", "/screen/main", "Switches the display to the Main screen."},
    {"POST", "/screen/settings", "Switches the display to the Settings screen."},
};
}  // namespace

void WebServerService::handleApiHelp() {
    sendHtmlBegin("API");

    server_.sendContent(
        "<p>Endpoints provided by this web server. JSON endpoints are safe to poll "
        "from scripts (curl, Home Assistant, etc.); POST endpoints change device state.</p>"
        "<table><tr><th>Method</th><th>Path</th><th>Description</th></tr>");

    char row[384];
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
        snprintf(row, sizeof(row), "<tr><td>%s</td><td>%s</td><td>%s</td></tr>", endpoint.method,
                 pathCell, endpoint.description);
        server_.sendContent(row);
    }
    server_.sendContent("</table>");

    server_.sendContent(
        "<p>Human-oriented pages: <code>/</code>, <code>/system-info</code>, "
        "<code>/app-info</code>, <code>/config</code> (active tuning constants), "
        "<code>/logs</code> (recent log entries).</p>");

    sendHtmlEnd();
}

// ---------------------------------------------------------------------------
// /api/raw — last NerdWinSense payload, unfiltered.
// ---------------------------------------------------------------------------

void WebServerService::handleApiRaw() {
    String raw;
    if (!pcMetricsService_.fetchRawJson(raw)) {
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

void WebServerService::handleApiPc() {
    char buf[256];

    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", "");

    snprintf(buf, sizeof(buf),
             "{\"available\":%s,\"last_update_timestamp\":%lu,"
             "\"cpu_temperature\":%u,\"gpu_temperature\":%u,"
             "\"cpu_load\":%u,\"gpu_load\":%u,\"mem_load\":%u,"
             "\"cpu_power\":%u,\"gpu_power\":%u,\"cpu_fan\":%u,\"gpu_fan\":%u,",
             pcMetrics_.is_available ? "true" : "false", pcMetrics_.last_update_timestamp,
             pcMetrics_.cpu_temperature, pcMetrics_.gpu_temperature, pcMetrics_.cpu_load,
             pcMetrics_.gpu_load, pcMetrics_.mem_load, pcMetrics_.cpu_power, pcMetrics_.gpu_power,
             pcMetrics_.cpu_fan, pcMetrics_.gpu_fan);
    server_.sendContent(buf);

    server_.sendContent("\"cpu_thread_load\":[");
    const int threadCount =
        sizeof(pcMetrics_.cpu_thread_load) / sizeof(pcMetrics_.cpu_thread_load[0]);
    for (int i = 0; i < threadCount; ++i) {
        snprintf(buf, sizeof(buf), "%s%u", i == 0 ? "" : ",", pcMetrics_.cpu_thread_load[i]);
        server_.sendContent(buf);
    }
    server_.sendContent("],\"system_fans\":[");
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count; ++i) {
        snprintf(buf, sizeof(buf), "%s%u", i == 0 ? "" : ",", pcMetrics_.system_fans[i]);
        server_.sendContent(buf);
    }
    server_.sendContent("],");

    snprintf(buf, sizeof(buf),
             "\"gpu_3d\":%u,\"gpu_compute\":%u,\"gpu_decode\":%u,\"gpu_mem\":%u,"
             "\"gpu_fps\":%d,\"eth_up\":%.2f,\"eth_dn\":%.2f,\"disk_drives\":[",
             pcMetrics_.gpu_3d, pcMetrics_.gpu_compute, pcMetrics_.gpu_decode, pcMetrics_.gpu_mem,
             pcMetrics_.gpu_fps, pcMetrics_.eth_up, pcMetrics_.eth_dn);
    server_.sendContent(buf);

    {
        // disk_drives is written from core 0 — must hold the lock for the
        // duration of the read, same as any other disk_drives access.
        PcMetricsDiskLock lock(pcMetrics_);
        for (size_t i = 0; i < pcMetrics_.disk_drives.size(); ++i) {
            const DiskDrive& drive = pcMetrics_.disk_drives[i];
            snprintf(buf, sizeof(buf),
                     "%s{\"name\":\"%s\",\"free_percent\":%.1f,\"read_kbps\":%.1f,"
                     "\"write_kbps\":%.1f}",
                     i == 0 ? "" : ",", drive.driveName, drive.freeSpacePercent, drive.readKBPerSec,
                     drive.writeKBPerSec);
            server_.sendContent(buf);
        }
    }
    server_.sendContent("]}");

    server_.sendContent("");  // flush / end chunked transfer
}

// ---------------------------------------------------------------------------
// /config — active AppSettings values, so a flashed device can be confirmed
// to be running the tuning constants (and debug/release build) expected.
// ---------------------------------------------------------------------------

void WebServerService::handleConfig() {
    sendHtmlBegin("Config");
    char buf[128];

    server_.sendContent("<pre>");

#define SEND_CONFIG_U(name, value)                                    \
    do {                                                              \
        snprintf(buf, sizeof(buf), "%-32s %lu\n", name,               \
                 static_cast<unsigned long>(value));                  \
        server_.sendContent(buf);                                     \
    } while (0)
#define SEND_CONFIG_F(name, value)                              \
    do {                                                         \
        snprintf(buf, sizeof(buf), "%-32s %.3f\n", name, (value)); \
        server_.sendContent(buf);                                 \
    } while (0)
#define SEND_CONFIG_B(name, value)                                          \
    do {                                                                    \
        snprintf(buf, sizeof(buf), "%-32s %s\n", name, (value) ? "true" : "false"); \
        server_.sendContent(buf);                                           \
    } while (0)

    SEND_CONFIG_U("debugSerialBaudRate", config_.debugSerialBaudRate);
    SEND_CONFIG_U("debugSerialTimeoutMs", config_.debugSerialTimeoutMs);
    SEND_CONFIG_B("debugWaitForSerial", config_.debugWaitForSerial);
    SEND_CONFIG_U("initNetworkRetries", config_.initNetworkRetries);
    SEND_CONFIG_U("initNetworkRetryDelayMs", config_.initNetworkRetryDelayMs);
    SEND_CONFIG_U("initTimeSyncRetries", config_.initTimeSyncRetries);
    SEND_CONFIG_U("initTimeSyncBaseDelayMs", config_.initTimeSyncBaseDelayMs);
    SEND_CONFIG_U("initBackoffJitterMs", config_.initBackoffJitterMs);
    SEND_CONFIG_U("watchdogTimeoutMs", config_.watchdogTimeoutMs);
    SEND_CONFIG_B("watchdogEnableOnBoot", config_.watchdogEnableOnBoot);
    SEND_CONFIG_U("timingScreenTaskMs", config_.timingScreenTaskMs);
    SEND_CONFIG_U("timingBackgroundTaskMs", config_.timingBackgroundTaskMs);
    SEND_CONFIG_U("timingMainLoopMs", config_.timingMainLoopMs);
    SEND_CONFIG_U("tasksScreenStack", config_.tasksScreenStack);
    SEND_CONFIG_U("tasksBackgroundStack", config_.tasksBackgroundStack);
    SEND_CONFIG_U("tasksScreenPriority", config_.tasksScreenPriority);
    SEND_CONFIG_U("tasksBackgroundPriority", config_.tasksBackgroundPriority);
    SEND_CONFIG_U("hardwareMonitorRefreshMs", config_.hardwareMonitorRefreshMs);
    SEND_CONFIG_U("hardwareMonitorThreadsRefreshMs", config_.hardwareMonitorThreadsRefreshMs);
    SEND_CONFIG_U("hardwareMonitorFailureRefreshMs", config_.hardwareMonitorFailureRefreshMs);
    SEND_CONFIG_U("hardwareMonitorRetryDelayMs", config_.hardwareMonitorRetryDelayMs);
    SEND_CONFIG_U("hardwareMonitorMaxRetries", config_.hardwareMonitorMaxRetries);
    SEND_CONFIG_F("hardwareMonitorThreadsUpwardSmoothing",
                  config_.hardwareMonitorThreadsUpwardSmoothing);
    SEND_CONFIG_F("hardwareMonitorThreadsDownwardSmoothing",
                  config_.hardwareMonitorThreadsDownwardSmoothing);
    SEND_CONFIG_U("airQualityFailureBackoffMs", config_.airQualityFailureBackoffMs);
    SEND_CONFIG_U("metricsMaxScreenDrawTimes", config_.metricsMaxScreenDrawTimes);
    SEND_CONFIG_U("pcMetricsCores", config_.pcMetricsCores);
    SEND_CONFIG_U("uiTransitionTimeoutMs", config_.uiTransitionTimeoutMs);
    SEND_CONFIG_U("uiTouchDebounceIntervalMs", config_.uiTouchDebounceIntervalMs);
    SEND_CONFIG_U("uiScreenTransitionCooldownMs", config_.uiScreenTransitionCooldownMs);
    SEND_CONFIG_U("uiDisplayLockTimeoutMs", config_.uiDisplayLockTimeoutMs);

#undef SEND_CONFIG_U
#undef SEND_CONFIG_F
#undef SEND_CONFIG_B

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

const char* WebServerService::logLevelToString(LoggerInterface::LogLevel level) {
    switch (level) {
        case LoggerInterface::LogLevel::DEBUG: return "DEBUG";
        case LoggerInterface::LogLevel::INFO: return "INFO";
        case LoggerInterface::LogLevel::WARNING: return "WARNING";
        case LoggerInterface::LogLevel::ERROR: return "ERROR";
        case LoggerInterface::LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void WebServerService::handleLogs() {
    sendHtmlBegin("Logs");

    // Heap-allocated — kRecentLogCapacity * sizeof(LogEntry) is too big for a
    // comfortable stack frame on the Arduino loop task.
    auto entries = std::make_unique<LoggerInterface::LogEntry[]>(LoggerInterface::kRecentLogCapacity);
    const size_t count = logger_.copyRecentLogs(entries.get(), LoggerInterface::kRecentLogCapacity);

    char row[256];
    if (count == 0) {
        server_.sendContent("<p>No log entries recorded yet.</p>");
    } else {
        server_.sendContent("<table><tr><th>Time</th><th>Level</th><th>Message</th></tr>");
        for (size_t i = 0; i < count; ++i) {
            const LoggerInterface::LogEntry& entry = entries[i];
            snprintf(row, sizeof(row), "<tr><td>%s</td><td>%s</td><td>%s</td></tr>",
                     entry.timestamp, logLevelToString(entry.level), entry.message);
            server_.sendContent(row);
        }
        server_.sendContent("</table>");
    }

    sendHtmlEnd();
}
