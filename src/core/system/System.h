#ifndef SYSTEM_H
#define SYSTEM_H

#include <WebServer.h>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/TaskManager.h"
#include "core/network/NetworkManager.h"
#include "core/system/SystemInit.h"
#include "core/system/SystemState.h"
#include "display/ScreenManager.h"
#include "display/ScreenTypes.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "services/PcMetricsService.h"
#include "utils/Logger.h"

class System {
   public:
    System();
    ~System() = default;

    // Delete copy/move operations
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = delete;
    System& operator=(System&&) = delete;

    bool initialize();
    void run();

   private:
    // Component Initialization
    bool initializeSubsystems();
    void handleInitializationFailure();

    // System Components
    LGFX display_;
    DisplayManager displayManager_;
    WebServer webServer_;

    // Managers and Services
    SystemState systemState_;
    Logger logger_;
    ScreenManager screenManager_;
    NetworkManager networkManager_;
    NtpService ntpService_;
    HttpServer httpServer_;
    PcMetricsService hmDataService_;
    TaskManager taskManager_;
};
#endif  // SYSTEM_H