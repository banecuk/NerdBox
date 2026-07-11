#pragma once

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "HttpClient.h"
#include "utils/Logger.h"

class NetworkManager {
 public:
    NetworkManager(LoggerInterface& logger, HttpClient& httpClient, const AppSettings& config);
    bool connect();
    bool checkAndReconnect();
    bool isConnected() const;
    String getLocalIp() const { return isConnected() ? WiFi.localIP().toString() : ""; }

    HttpClient& getHttpClient() { return httpClient_; }

 private:
    static constexpr uint32_t kReconnectTimeoutMs      = 10000;
    static constexpr uint32_t kReconnectCheckIntervalMs = 15000;

    void startReconnect(uint32_t now);

    LoggerInterface& logger_;
    HttpClient& httpClient_;
    const AppSettings& config_;

    uint32_t lastReconnectAttemptMs_ = 0;
    uint32_t reconnectStartMs_       = 0;
    uint32_t reconnectAttempts_      = 0;
    bool reconnecting_               = false;
};