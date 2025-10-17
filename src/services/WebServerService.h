#pragma once

#include <WebServer.h>

#include "ui/UiController.h"

class WebServerService {
   public:
    WebServerService(UiController& uiController, ApplicationMetrics& systemMetrics);
    void begin();
    void processRequests();

   private:
    WebServer server_;
    UiController& uiController_;
    ApplicationMetrics& systemMetrics_;

    void handleNotFound();
    void handleHome();
    void handleSystemInfo();
    void handleAppInfo();

    String getSystemInfo();
    String getAppInfo();

    String wrapHtmlContent(const String& title, const String& content);
};