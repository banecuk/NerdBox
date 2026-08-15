#pragma once

#include <esp_task_wdt.h>

#include <memory>

#include "app/ApplicationComponents.h"
#include "config/AppSettings.h"
#include "services/web/WebServerService.h"

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

    // Lets main.cpp read config (e.g. debug serial settings) after
    // constructing Application, instead of keeping a second AppSettings
    // instance alive at file scope just for pre-initialize() use.
    const AppSettings& config() const { return config_; }

 private:
    std::unique_ptr<ApplicationComponents> components_;

    // Injected references to the specific runtime dependencies of run().
    // These are extracted from components_ in the constructor so that run()
    // does not need to reach into the composition root on every loop tick.
    const AppSettings& config_;
    WebServerService& webServerService_;
};