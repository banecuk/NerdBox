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
    bool isConnected() const;
    String get(const String& url);
    String getLocalIp() const { return isConnected() ? WiFi.localIP().toString() : ""; }

    HttpClient& getHttpClient() { return httpClient_; }

 private:
    LoggerInterface& logger_;
    HttpClient& httpClient_;
    AppConfigInterface& config_;
};