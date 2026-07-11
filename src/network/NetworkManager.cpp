#include "NetworkManager.h"

NetworkManager::NetworkManager(LoggerInterface& logger, HttpClient& httpClient,
                               const AppSettings& config)
    : logger_(logger), httpClient_(httpClient), config_(config) {}

bool NetworkManager::connect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    logger_.info("Connecting to WiFi...", true);

    uint32_t timeoutMs = config_.initNetworkRetries * config_.initNetworkRetryDelayMs;
    wl_status_t status =
        static_cast<wl_status_t>(WiFi.waitForConnectResult(timeoutMs));
    bool connected = (status == WL_CONNECTED);

    if (connected) {
        char msg[64];
        snprintf(msg, sizeof(msg), "WiFi connected - IP: %s",
                WiFi.localIP().toString().c_str());
        logger_.info(msg, true);
        reconnectAttempts_ = 0;
    } else {
        logger_.errorf("WiFi failed, status: %d", status);
    }

    return connected;
}

void NetworkManager::startReconnect(uint32_t now) {
    reconnectAttempts_++;
    logger_.infof("WiFi lost — reconnect attempt %u...", reconnectAttempts_);

    // Cleanly tear down before re-joining; false = keep credentials
    WiFi.disconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    reconnecting_     = true;
    reconnectStartMs_ = now;
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

/**
 * Call this periodically from the background task (e.g. every tick).
 * Non-blocking: kicks off WiFi.begin() once when the link drops, then polls
 * WiFi.status() on subsequent calls instead of busy-waiting. Returns true if
 * the link is (or just became) up, false if it is still down.
 */
bool NetworkManager::checkAndReconnect() {
    if (isConnected()) {
        if (reconnecting_) {
            logger_.infof("Reconnected (attempt %u) — IP: %s",
                          reconnectAttempts_,
                          WiFi.localIP().toString().c_str());
            reconnecting_      = false;
            reconnectAttempts_ = 0;
        }
        return true;
    }

    const uint32_t now = millis();

    if (!reconnecting_) {
        // Back off — do not hammer the WiFi stack
        if (now - lastReconnectAttemptMs_ < kReconnectCheckIntervalMs) {
            return false;
        }
        lastReconnectAttemptMs_ = now;
        startReconnect(now);
        return false;
    }

    if (now - reconnectStartMs_ >= kReconnectTimeoutMs) {
        logger_.errorf("Reconnect attempt %u timed out after %u ms",
                       reconnectAttempts_, kReconnectTimeoutMs);
        reconnecting_           = false;
        lastReconnectAttemptMs_ = now;
    }

    return false;
}
