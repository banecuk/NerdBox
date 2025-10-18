#pragma once

#include <esp_task_wdt.h>
#include <WebServer.h>

#include <memory>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "config/AppConfigService.h"
#include "core/state/SystemState.h"
#include "core/TaskManager.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/NtpService.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "services/WebServerService.h"
#include "ui/Colors.h"
#include "ui/DisplayManager.h"
#include "ui/screens/ScreenTypes.h"
#include "ui/UIController.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

// Forward declaration
class ApplicationComponents;

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
};