#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppConfig.h"
#include "config/Environment.h"
#include "utils/Logger.h"
#include "HttpClient.h"

class NetworkManager {
   public:
    NetworkManager(ILogger &logger, HttpClient &httpClient);
    bool connect();
    bool isConnected() const;
    String get(const String &url);

    HttpClient& getHttpClient() { return httpClient_; }

   private:
    ILogger &logger_;
    HttpClient &httpClient_; // TODO rename to HttpClient or something more generic

    bool connected_ = false;
};

#endif