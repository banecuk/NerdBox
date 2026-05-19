#pragma once

#include <WebServer.h>

#include "ui/core/UiController.h"

class WebServerService {
 public:
    WebServerService(WebServer& server, UiController& uiController,
                     ApplicationMetrics& systemMetrics);
    void begin();
    void processRequests();

 private:
    WebServer& server_;
    UiController& uiController_;
    ApplicationMetrics& systemMetrics_;

    void handleNotFound();
    void handleHome();
    void handleSystemInfo();
    void handleAppInfo();

    // Streams the HTML wrapper and body content directly to the client in
    // chunks — no large String assembled on the heap.
    void sendHtmlBegin(const char* title);
    void sendHtmlEnd();
    void sendSystemInfoBody();
    void sendAppInfoBody();
};