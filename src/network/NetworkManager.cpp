#include "NetworkManager.h"

#include <ESPmDNS.h>

NetworkManager::NetworkManager(LoggerInterface& logger, HttpClient& httpClient,
                               const AppSettings& config)
    : logger_(logger), httpClient_(httpClient), config_(config) {}

bool NetworkManager::connect() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    logger_.info("Connecting to WiFi...", true);

    // waitForConnectResult() blocks once for the whole timeout below — it
    // does not retry internally. initNetworkRetries doesn't cause repeated
    // connect attempts here; despite the name, it's just a multiplier used
    // to size this one overall timeout (retries * per-retry delay). The
    // background task's own reconnect loop, checkAndReconnect(), uses its
    // own separate kReconnectTimeoutMs/kReconnectCheckIntervalMs constants,
    // not these config fields.
    uint32_t timeoutMs = config_.initNetworkRetries * config_.initNetworkRetryDelayMs;
    wl_status_t status = static_cast<wl_status_t>(WiFi.waitForConnectResult(timeoutMs));
    bool connected = (status == WL_CONNECTED);

    if (connected) {
        char msg[64];
        snprintf(msg, sizeof(msg), "WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
        logger_.info(msg, true);
        reconnectAttempts_ = 0;
        startMdns();
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

    reconnecting_ = true;
    reconnectStartMs_ = now;
}

bool NetworkManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::startMdns() {
    // end() is safe to call even if begin() was never called — needed here
    // since a reconnect re-invokes this on a responder that may already be
    // bound to the old IP.
    MDNS.end();
    if (MDNS.begin(config_.networkMdnsHostname)) {
        MDNS.addService("http", "tcp", 80);
        logger_.infof("mDNS responder started: http://%s.local", config_.networkMdnsHostname);
    } else {
        logger_.error("mDNS responder failed to start");
    }
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
            logger_.infof("Reconnected (attempt %u) — IP: %s", reconnectAttempts_,
                          WiFi.localIP().toString().c_str());
            reconnecting_ = false;
            reconnectAttempts_ = 0;
            startMdns();
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
        logger_.errorf("Reconnect attempt %u timed out after %u ms", reconnectAttempts_,
                       kReconnectTimeoutMs);
        reconnecting_ = false;
        lastReconnectAttemptMs_ = now;
    }

    return false;
}
