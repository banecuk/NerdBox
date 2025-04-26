#ifndef SYSTEM_H
#define SYSTEM_H

#include <WebServer.h>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/display/ScreenManager.h"
#include "core/display/ScreenTypes.h"
#include "core/network/NetworkManager.h"
#include "core/system/SystemInit.h"
#include "core/system/SystemState.h"
#include "core/utils/Logger.h"
#include "core/utils/TaskManager.h"
#include "services/PcMetricsService.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"

class System {
   public:
    System();
    ~System() = default;

    // Delete copy/move operations
    System(const System &) = delete;
    System &operator=(const System &) = delete;
    System(System &&) = delete;
    System &operator=(System &&) = delete;

    bool initialize();
    void run();

   private:
    // System components
    LGFX display_;
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

    // Private Methods
    void initializeSerial();
    bool initializeSubsystems();
    bool initializeNetwork(uint8_t maxRetries);
    bool initializeTimeService(uint8_t maxRetries);
    bool initializeDisplay(uint8_t maxRetries);
    void postDisplayInitialization();
    void handleInitializationFailure();
    void handleTouchInput();
    void updateScreenTask(void *parameter);
    void backgroundTask(void *parameter);
    bool createTasks();
    void handleDownloadFailure(uint8_t &consecutiveFailures);
};

#endif  // SYSTEM_H