#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <esp_task_wdt.h>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/display/ScreenManager.h"
#include "core/network/NetworkManager.h"
#include "core/utils/Logger.h"
#include "core/utils/TaskManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"

class SystemInit {
   public:
    SystemInit(ILogger& logger, SystemState::CoreState& coreState_, LGFX& display,
               ScreenManager& screenManager, NetworkManager& networkManager,
               NtpService& ntpService, TaskManager& taskManager, HttpServer& httpServer,
               SystemState::ScreenState& screenState);

    bool initializeAll();

   private:
    enum class State {
        INITIAL,
        SERIAL_INIT,
        DISPLAY_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        WATCHDOG_INIT,
        FINAL_SETUP,
        COMPLETE,
        FAILED
    };

    // Initialization methods
    bool initializeSerial();
    bool initializeDisplay(uint8_t maxRetries);
    bool initializeNetwork(uint8_t maxRetries);
    bool initializeTimeService(uint8_t maxRetries);
    bool initializeWatchdog();
    void postDisplayInitialization();

    // State machine helpers
    void transitionTo(State newState);
    void performFinalSetup();
    void logCompletionStatus(bool success);
    String getStateName(State state);

    // Helper methods
    void logRetryAttempt(const char* component, uint8_t attempt, uint8_t maxRetries);
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay);
    void adjustDisplayOrientation();

    // References
    SystemState::CoreState& coreState_;
    ILogger& logger_;
    LGFX& display_;
    ScreenManager& screenManager_;
    NetworkManager& networkManager_;
    NtpService& ntpService_;
    TaskManager& taskManager_;
    HttpServer& httpServer_;
    SystemState::ScreenState& screenState_;

    State currentState_;
};

#endif