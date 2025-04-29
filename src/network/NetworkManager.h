#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <HTTPClient.h>
#include <WiFi.h>

#include "config/AppConfig.h"
#include "config/Environment.h"
#include "utils/Logger.h"
#include "HttpDownloader.h"

class NetworkManager {
   public:
    NetworkManager(ILogger &logger, HttpDownloader &httpDownloader);
    bool connect();
    bool isConnected() const;
    String get(const String &url);

    HttpDownloader& getHttpDownloader() { return httpDownloader_; }

   private:
    ILogger &logger_;
    HttpDownloader &httpDownloader_; // TODO rename to HttpClient or something more generic

    bool connected_ = false;
};

#endif