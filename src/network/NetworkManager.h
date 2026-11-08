#pragma once

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppSettings.h"
#include "network/HttpClient.h"
#include "network/LinkInfo.h"
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

    HttpClient& getHttpClient() { return httpClient_; }

    // Link details for the WIFI screen (WifiLinkWidget) — see
    // docs-local/13-wifi-screen-plan.md §2.2. Deliberately not a DataBundle
    // member: wanted by exactly one widget, cheap to read on demand
    // (~190 B on the caller's stack), so growing ApplicationComponents for it
    // would cost more than it buys (see the ApplicationComponents.h
    // size-sensitivity note).
    // Fills `out` with the current link's details — all-empty/zero and
    // connected=false when not connected. No String allocation. Queries the
    // WiFi driver, so call it from the background task only: everyone else
    // reads the snapshot NetworkStatusService publishes into NetworkStatus.
    void fillLinkInfo(LinkInfo& out) const;

 private:
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