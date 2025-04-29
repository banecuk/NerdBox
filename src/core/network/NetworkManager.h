#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppConfig.h"
#include "config/Environment.h"
#include "utils/Logger.h"

class NetworkManager {
   public:
    NetworkManager(ILogger &logger);
    bool connect();
    bool isConnected() const;
    String get(const String &url);

   private:
    ILogger &logger_;

    bool connected_ = false;
};

#endif