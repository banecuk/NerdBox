#pragma once

#include <WebServer.h>

#include "services/web/WebApiHandlers.h"
#include "services/web/WebDataSources.h"
#include "services/web/WebPageHandlers.h"

// Routing only: registers every route with the underlying WebServer and
// dispatches to WebApiHandlers (JSON/Prometheus endpoints) or
// WebPageHandlers (human-facing HTML pages). /restart is the one handler
// that stays here — it's neither JSON nor a rendered page, just an
// EventBus publish.
class WebServerService {
 public:
    // sources is copied (it's a struct of references), so the caller's
    // instance may be a temporary — see ApplicationComponents.cpp.
    WebServerService(WebServer& server, const WebDataSources& sources);
    void begin();
    void processRequests();

 private:
    WebServer& server_;
    // Declared before the handlers, which hold a reference to it.
    const WebDataSources sources_;
    WebApiHandlers apiHandlers_;
    WebPageHandlers pageHandlers_;

    void handleRestart();
    void handleAudioPush();
};