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

 private:
    static constexpr uint32_t kReconnectTimeoutMs = 10000;
    static constexpr uint32_t kReconnectCheckIntervalMs = 15000;

    void startReconnect(uint32_t now);

    LoggerInterface& logger_;
    HttpClient& httpClient_;
    const AppSettings& config_;

    uint32_t lastReconnectAttemptMs_ = 0;
    uint32_t reconnectStartMs_ = 0;
    uint32_t reconnectAttempts_ = 0;
    bool reconnecting_ = false;
};