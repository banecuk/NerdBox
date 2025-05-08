#include "HttpServer.h"

HttpServer::HttpServer(UIController& uiController)
    : server_(80), uiController_(uiController) {}

void HttpServer::begin() {
    server_.on("/", [this]() { this->handleHome(); });
    server_.on("/system-info", [this]() { this->handleSystemInfo(); });
    server_.on("/screen/main", [this]() { 
        uiController_.setScreen(ScreenName::MAIN); });
    server_.on("/screen/settings", [this]() { 
        uiController_.setScreen(ScreenName::SETTINGS); });
    server_.onNotFound([this]() { this->handleNotFound(); });
    server_.begin();
}

void HttpServer::processRequests() { server_.handleClient(); }

void HttpServer::handleNotFound() { server_.send(404, "text/plain", "Not found"); }

void HttpServer::handleHome() {
    server_.send(200, "text/html", wrapHtmlContent("Homepage", ""));
}

String HttpServer::getSystemInfo() {
    String info;
    info.reserve(300);

    info += "<pre>";
    info += "Free Heap: ";
    info += ESP.getFreeHeap();
    info += " bytes\n";

    info += "PSRAM Size: ";
    info += ESP.getPsramSize();
    info += " bytes\n";

    info += "PSRAM Free: ";
    info += ESP.getFreePsram();
    info += " bytes\n";

    info += "CPU Frequency: ";
    info += ESP.getCpuFreqMHz();
    info += " MHz\n";

    info += "SDK Version: ";
    info += ESP.getSdkVersion();
    info += "</pre>";

    return wrapHtmlContent("System Information", info);
}

void HttpServer::handleSystemInfo() { server_.send(200, "text/html", getSystemInfo()); }

String HttpServer::wrapHtmlContent(const String &title, const String &content) {
    String html;
    html.reserve(800 +
                 content.length());  // Increased reserve for menu and additional styles

    html += "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>";
    html += title;
    html += " - NerdBox</title>";
    html += "<style>";
    html += "body {";
    html += "  font-family: 'Segoe UI', Arial, sans-serif;";
    html += "  margin: 0;";
    html += "  padding: 0;";
    html += "  line-height: 1.6;";
    html += "  color: #333;";
    html += "}";
    html += "header {";
    html += "  background: #2c3e50;";
    html += "  color: white;";
    html += "  padding: 1rem 0;";
    html += "  margin-bottom: 1.5rem;";
    html += "  box-shadow: 0 2px 5px rgba(0,0,0,0.1);";
    html += "}";
    html += ".header-content {";
    html += "  max-width: 1200px;";
    html += "  margin: 0 auto;";
    html += "  padding: 0 1rem;";
    html += "}";
    html += "nav {";
    html += "  margin-top: 1rem;";
    html += "}";
    html += "nav ul {";
    html += "  list-style: none;";
    html += "  padding: 0;";
    html += "  margin: 0;";
    html += "  display: flex;";
    html += "  gap: 1rem;";
    html += "}";
    html += "nav a {";
    html += "  color: white;";
    html += "  text-decoration: none;";
    html += "  padding: 0.5rem 1rem;";
    html += "  border-radius: 4px;";
    html += "  transition: background-color 0.3s;";
    html += "}";
    html += "nav a:hover {";
    html += "  background-color: #34495e;";
    html += "}";
    html += "h1 { margin: 0; }";
    html += ".content {";
    html += "  max-width: 1200px;";
    html += "  margin: 0 auto;";
    html += "  padding: 0 1rem;";
    html += "}";
    html += "footer {";
    html += "  background: #f8f9fa;";
    html += "  margin-top: 2rem;";
    html += "  padding: 1.5rem 0;";
    html += "  border-top: 1px solid #ddd;";
    html += "  color: #666;";
    html += "  text-align: center;";
    html += "}";
    html += "pre {";
    html += "  background: #f8f9fa;";
    html += "  padding: 1.5rem;";
    html += "  border-radius: 5px;";
    html += "  overflow-x: auto;";
    html += "  border: 1px solid #ddd;";
    html += "}";
    html += "</style></head>";
    html += "<body>";
    html += "<header>";
    html += "<div class='header-content'>";
    html += "<span>NerdBox</span>";
    html += "<h1>";
    html += title;
    html += "</h1>";
    html += "<nav>";
    html += "<ul>";
    html += "<li><a href='/'>Home</a></li>";
    html += "<li><a href='/system-info'>System Info</a></li>";
    html += "</ul>";
    html += "</nav>";
    html += "</div>";
    html += "</header>";
    html += "<div class='content'>";
    html += content;
    html += "</div>";
    html += "<footer>NerdBox&copy; 2025<br /><small>WT32-SC01-PLUS</small></footer>";
    html += "</body></html>";

    return html;
}
