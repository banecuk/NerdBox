#include "NetworkManager.h"


NetworkManager::NetworkManager(ILogger &logger, HttpClient &httpClient)
        : logger_(logger), httpClient_(httpClient) {}

bool NetworkManager::connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED &&
           attempts < Config::Init::kDefaultNetworkRetries) {
        delay(Config::Init::kNetworkRetryDelayMs);
        attempts++;

        logger_.warning("Retry connect...", true);
    }

    connected_ = (WiFi.status() == WL_CONNECTED);
    if (connected_) {
        String ipAddress = WiFi.localIP().toString();
        logger_.info("WiFi connected - IP: " + ipAddress, true);
    } else {
        logger_.error("WiFi connection failed", true);
    }
    return connected_;
}

bool NetworkManager::isConnected() const { return connected_; }

String NetworkManager::get(const String &url) {
    if (!connected_) return "";

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        return http.getString();
    }
    return "";
}