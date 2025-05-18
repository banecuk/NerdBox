#pragma once

#include <WebServer.h>

#include "ui/UIController.h"

class HttpServer {
   public:
    HttpServer(UIController& uiController, ApplicationMetrics& systemMetrics);
    void begin();
    void processRequests();

   private:
    WebServer server_;
    UIController& uiController_;
    ApplicationMetrics& systemMetrics_;

    void handleNotFound();
    void handleHome();
    void handleSystemInfo();
    void handleAppInfo();

    String getSystemInfo();
    String getAppInfo();

    String wrapHtmlContent(const String& title, const String& content);
};