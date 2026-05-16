#pragma once

#include <esp_task_wdt.h>

#include <memory>

#include "config/AppConfigInterface.h"
#include "core/ApplicationComponents.h"
#include "core/state/SystemState.h"
#include "services/WebServerService.h"

class Application {
 public:
    explicit Application(std::unique_ptr<ApplicationComponents> components);
    ~Application() = default;

    // Delete copy/move operations
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool initialize();
    void run();

 private:
    std::unique_ptr<ApplicationComponents> components_;

    // Injected references to the specific runtime dependencies of run().
    // These are extracted from components_ in the constructor so that run()
    // does not need to reach into the composition root on every loop tick.
    AppConfigInterface&  config_;
    SystemState&         systemState_;
    WebServerService&    webServerService_;
};