#pragma once

#include <HTTPClient.h>
#include <WiFi.h>

#include "HttpClient.h"
#include "config/AppConfig.h"
#include "config/Environment.h"
#include "utils/Logger.h"

class NetworkManager {
   public:
    NetworkManager(LoggerInterface &logger, HttpClient &httpClient);
    bool connect();
    bool isConnected() const;
    String get(const String &url);

    HttpClient &getHttpClient() { return httpClient_; }

   private:
    LoggerInterface &logger_;
    HttpClient &httpClient_;  // TODO rename to HttpClient or something more generic

    bool connected_ = false;
};