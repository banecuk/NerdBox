#pragma once

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "network/HttpClient.h"
#include "utils/logging/Logger.h"

class NetworkManager {
 public:
    NetworkManager(LoggerInterface& logger, HttpClient& httpClient, const AppSettings& config);
    bool connect();
    bool checkAndReconnect();
    bool isConnected() const;

    // True while checkAndReconnect() is mid-(re)association. WifiScanJob uses
    // this to avoid starting a scan during association — scanning off the
    // home channel while WiFi.begin() is trying to (re)join can make the
    // connect attempt itself fail (see docs-local/13-wifi-screen-plan.md §3.3).
    bool isReconnecting() const { return reconnecting_; }

    // Writes the dotted-quad IP into the caller's buffer (empty string if not
    // connected) — no String allocation, unlike IPAddress::toString().
    void getLocalIp(char* buf, size_t size) const {
        if (!isConnected() || size == 0) {
            if (size > 0)
                buf[0] = '\0';
            return;
        }
        IPAddress ip = WiFi.localIP();
        snprintf(buf, size, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    }

    HttpClient& getHttpClient() { return httpClient_; }

    // Link details for the WIFI screen (WifiLinkWidget) — see
    // docs-local/13-wifi-screen-plan.md §2.2. Deliberately not a DataBundle
    // member: wanted by exactly one widget, cheap to read on demand
    // (~190 B on the caller's stack), so growing ApplicationComponents for it
    // would cost more than it buys (see the ApplicationComponents.h
    // size-sensitivity note).
    struct LinkInfo {
        char ssid[33];
        char ip[16];
        char gateway[16];
        char mask[16];
        char dns[16];
        char mac[18];       // AA:BB:CC:DD:EE:FF
        char hostname[64];  // config_.networkMdnsHostname + ".local"
        uint8_t bssid[6];
        int8_t rssi;
        uint8_t channel;
        uint8_t auth;
        bool connected;
    };

    // Fills `out` with the current link's details — all-empty/zero and
    // connected=false when not connected. No String allocation.
    void fillLinkInfo(LinkInfo& out) const;

 private:
    static constexpr uint32_t kReconnectTimeoutMs = 10000;
    static constexpr uint32_t kReconnectCheckIntervalMs = 15000;

    void startReconnect(uint32_t now);

    // (Re)starts the mDNS responder so the device stays reachable as
    // <hostname>.local across reconnects — ESP32's mDNS responder doesn't
    // survive a WiFi drop/rejoin on its own, so this is called after both
    // the initial connect and every successful reconnect.
    void startMdns();

    LoggerInterface& logger_;
    HttpClient& httpClient_;
    const AppSettings& config_;

    uint32_t lastReconnectAttemptMs_ = 0;
    uint32_t reconnectStartMs_ = 0;
    uint32_t reconnectAttempts_ = 0;
    bool reconnecting_ = false;
};