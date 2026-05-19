#include "NetworkManager.h"

NetworkManager::NetworkManager(LoggerInterface& logger, HttpClient& httpClient,
                               AppConfigInterface& config)
    : logger_(logger), httpClient_(httpClient), config_(config) {}

bool NetworkManager::connect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    logger_.info("Connecting to WiFi...", true);

    uint32_t timeoutMs =
        config_.getInitNetworkRetries() * config_.getInitNetworkRetryDelayMs();
    wl_status_t status =
        static_cast<wl_status_t>(WiFi.waitForConnectResult(timeoutMs));
    bool connected = (status == WL_CONNECTED);

    if (connected) {
        logger_.info("WiFi connected - IP: " + WiFi.localIP().toString(), true);
        reconnectAttempts_ = 0;
    } else {
        logger_.errorf("WiFi failed, status: %d", status);
    }

    return connected;
}

bool NetworkManager::reconnect() {
    reconnectAttempts_++;
    logger_.infof("WiFi lost — reconnect attempt %u...", reconnectAttempts_);

    // Cleanly tear down before re-joining; false = keep credentials
    WiFi.disconnect(false);
    vTaskDelay(pdMS_TO_TICKS(kReconnectPreDelayMs));

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= kReconnectTimeoutMs) {
            logger_.errorf("Reconnect attempt %u timed out after %u ms",
                           reconnectAttempts_, kReconnectTimeoutMs);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    logger_.infof("Reconnected (attempt %u) — IP: %s",
                  reconnectAttempts_,
                  WiFi.localIP().toString().c_str());
    reconnectAttempts_ = 0;
    return true;
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

/**
 * Call this periodically from the background task (e.g. every
 * kReconnectCheckIntervalMs). Returns true if the link is (or just became)
 * up, false if it is still down after a reconnect attempt.
 */
bool NetworkManager::checkAndReconnect() {
    if (isConnected()) {
        return true;
    }

    const uint32_t now = millis();
    if (now - lastReconnectAttemptMs_ < kReconnectCheckIntervalMs) {
        // Back off — do not hammer the WiFi stack
        return false;
    }
    lastReconnectAttemptMs_ = now;

    return reconnect();
}

String NetworkManager::get(const String& url) {
    if (!isConnected())
        return "";

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    String body = "";
    if (httpCode == HTTP_CODE_OK) {
        body = http.getString();
    } else {
        logger_.errorf("HTTP GET failed, code: %d  url: %s",
                       httpCode, url.c_str());
    }
    http.end();  // always release connection
    return body;
}
