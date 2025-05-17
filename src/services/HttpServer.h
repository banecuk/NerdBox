#pragma once

#include <WebServer.h>

#include "ui/UIController.h"

class HttpServer {
   public:
    HttpServer(UIController& uiController);
    void begin();
    void processRequests();

   private:
    WebServer server_;
    UIController& uiController_;

    void handleNotFound();
    void handleHome();
    void handleSystemInfo();

    String getSystemInfo();

    String wrapHtmlContent(const String& title, const String& content);
};