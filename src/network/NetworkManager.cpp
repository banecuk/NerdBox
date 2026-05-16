#include "NetworkManager.h"

NetworkManager::NetworkManager(LoggerInterface& logger, HttpClient& httpClient,
                               AppConfigInterface& config)
    : logger_(logger), httpClient_(httpClient), config_(config) {}

bool NetworkManager::connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    logger_.info("Connecting to WiFi...", true);

    // waitForConnectResult blocks until connected, failed, or timeout
    uint32_t timeoutMs = config_.getInitNetworkRetries() * config_.getInitNetworkRetryDelayMs();
    wl_status_t status = static_cast<wl_status_t>(WiFi.waitForConnectResult(timeoutMs));

    bool connected = (status == WL_CONNECTED);
    if (connected) {
        logger_.info("WiFi connected - IP: " + WiFi.localIP().toString(), true);
    } else {
        logger_.errorf("WiFi failed, status: %d", status);
    }
    return connected;
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::get(const String& url) {
    if (!isConnected())
        return "";
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        return http.getString();
    }
    return "";
}