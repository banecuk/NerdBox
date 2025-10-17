#pragma once

#include <WebServer.h>
#include <esp_task_wdt.h>

#include <LovyanGFX.hpp>
#include <memory>

#include "config/AppConfig.h"
#include "config/AppConfigService.h"
#include "core/TaskManager.h"
#include "core/state/SystemState.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "ui/Colors.h"
#include "ui/DisplayManager.h"
#include "ui/UIController.h"
#include "ui/screens/ScreenTypes.h"
#include "utils/ApplicationMetrics.h"
#include "utils/Logger.h"

// Forward declaration
class ApplicationComponents;

class Application {
   public:
    // Remove InitState enum - now in InitializationStateMachine

    // Updated constructor to accept injected components
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
    // All components are now owned via ApplicationComponents
    std::unique_ptr<ApplicationComponents> components_;

    // Remove all initialization state management methods
    // They are now handled by InitializationStateMachine
};