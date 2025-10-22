#include "NetworkManager.h"

NetworkManager::NetworkManager(LoggerInterface& logger, HttpClient& httpClient,
                               AppConfigInterface& config)
    : logger_(logger), httpClient_(httpClient), config_(config) {}

bool NetworkManager::connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < config_.getInitNetworkRetries()) {
        delay(config_.getInitNetworkRetryDelayMs());
        attempts++;
        logger_.warning("Retry connect...", true);
    }
    isConnected_ = (WiFi.status() == WL_CONNECTED);
    if (isConnected_) {
        IPAddress ip = WiFi.localIP();
        char ipBuffer[16];  // "255.255.255.255" + null
        snprintf(ipBuffer, sizeof(ipBuffer), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

        char logBuffer[64];
        snprintf(logBuffer, sizeof(logBuffer), "WiFi connected - IP: %s", ipBuffer);
        logger_.info(logBuffer, true);
    } else {
        logger_.error("WiFi connection failed", true);
    }
    return isConnected_;
}

bool NetworkManager::isConnected() const {
    return isConnected_;
}

String NetworkManager::get(const String& url) {
    if (!isConnected_)
        return "";
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        return http.getString();
    }
    return "";
}