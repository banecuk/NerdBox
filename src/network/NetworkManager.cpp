#include "NetworkManager.h"

#include <ESPmDNS.h>
#include <esp_wifi.h>

#include <cstring>

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

void NetworkManager::fillLinkInfo(LinkInfo& out) const {
    out.ssid[0] = '\0';
    out.ip[0] = '\0';
    out.gateway[0] = '\0';
    out.mask[0] = '\0';
    out.dns[0] = '\0';
    out.mac[0] = '\0';
    out.hostname[0] = '\0';
    memset(out.bssid, 0, sizeof(out.bssid));
    out.rssi = 0;
    out.channel = 0;
    out.auth = 0;
    out.connected = isConnected();

    if (!out.connected) {
        return;
    }

    // esp_wifi_sta_get_ap_info() gives channel/authmode/bssid in one call —
    // the Arduino WiFiSTAClass surface has no equivalent for "the network
    // we're currently on" (WiFi.encryptionType(i)/channel(i) are scan-result
    // accessors, indexed by the last scanNetworks() call, not our own link).
    wifi_ap_record_t apInfo{};
    if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
        snprintf(out.ssid, sizeof(out.ssid), "%s", reinterpret_cast<const char*>(apInfo.ssid));
        memcpy(out.bssid, apInfo.bssid, sizeof(out.bssid));
        out.rssi = static_cast<int8_t>(apInfo.rssi);
        out.channel = apInfo.primary;
        out.auth = static_cast<uint8_t>(apInfo.authmode);
    }

    IPAddress ip = WiFi.localIP();
    snprintf(out.ip, sizeof(out.ip), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    IPAddress gw = WiFi.gatewayIP();
    snprintf(out.gateway, sizeof(out.gateway), "%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
    IPAddress mask = WiFi.subnetMask();
    snprintf(out.mask, sizeof(out.mask), "%u.%u.%u.%u", mask[0], mask[1], mask[2], mask[3]);
    IPAddress dns = WiFi.dnsIP();
    snprintf(out.dns, sizeof(out.dns), "%u.%u.%u.%u", dns[0], dns[1], dns[2], dns[3]);

    uint8_t macBuf[6] = {0};
    WiFi.macAddress(macBuf);
    snprintf(out.mac, sizeof(out.mac), "%02X:%02X:%02X:%02X:%02X:%02X", macBuf[0], macBuf[1],
             macBuf[2], macBuf[3], macBuf[4], macBuf[5]);

    snprintf(out.hostname, sizeof(out.hostname), "%s.local", config_.networkMdnsHostname);
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
