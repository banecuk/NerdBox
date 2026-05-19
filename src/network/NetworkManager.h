#pragma once

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppConfigInterface.h"
#include "config/Environment.h"
#include "HttpClient.h"
#include "utils/Logger.h"

class NetworkManager {
 public:
    NetworkManager(LoggerInterface& logger, HttpClient& httpClient, AppConfigInterface& config);
    bool connect();
    bool reconnect();
    bool checkAndReconnect();
    bool isConnected() const;
    String get(const String& url);
    String getLocalIp() const { return isConnected() ? WiFi.localIP().toString() : ""; }

    HttpClient& getHttpClient() { return httpClient_; }

 private:
    static constexpr uint32_t kReconnectTimeoutMs      = 10000;
    static constexpr uint32_t kReconnectPreDelayMs      = 300;
    static constexpr uint32_t kReconnectCheckIntervalMs = 15000;

    LoggerInterface& logger_;
    HttpClient& httpClient_;
    AppConfigInterface& config_;

    uint32_t lastReconnectAttemptMs_ = 0;
    uint32_t reconnectAttempts_      = 0;
};