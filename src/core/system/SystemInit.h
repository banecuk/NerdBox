#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <esp_task_wdt.h>

#include <LovyanGFX.hpp>

#include "config/AppConfig.h"
#include "core/TaskManager.h"
#include "network/NetworkManager.h"
#include "services/HttpServer.h"
#include "services/NtpService.h"
#include "ui/DisplayDriver.h"
#include "ui/UIController.h"
#include "utils/Logger.h"

namespace SystemInitStates {
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

constexpr const char* STATE_NAMES[] = {
    "INITIAL",   "SERIAL_INIT",   "DISPLAY_INIT", "TASKS_INIT", "NETWORK_INIT",
    "TIME_INIT", "WATCHDOG_INIT", "FINAL_SETUP",  "COMPLETE",   "FAILED"};
}  // namespace SystemInitStates

class SystemInit {
   public:
    using State = SystemInitStates::State;

    SystemInit(ILogger& logger, SystemState::CoreState& coreState, LGFX& display,
        DisplayDriver& displayDriver, UIController& uiController,
        NetworkManager& networkManager, HttpDownloader& httpDownloader,
        NtpService& ntpService,
        TaskManager& taskManager, HttpServer& httpServer,
        SystemState::ScreenState& screenState);

    bool initializeAll();

   private:
    // Initialization Methods
    bool initializeSerial();
    bool initializeDisplay();
    bool initializeNetwork(uint8_t maxRetries);
    bool initializeTimeService(uint8_t maxRetries);
    bool initializeWatchdog();
    void performFinalSetup();

    // State Management
    void transitionTo(State newState);
    bool handleStateTransition();
    bool isTerminalState() const;

    // Logging Helpers
    void logCompletionStatus(bool success) const;
    String getStateName(State state) const {
        return SystemInitStates::STATE_NAMES[static_cast<int>(state)];
    }
    void logRetryAttempt(const char* component, uint8_t attempt,
                         uint8_t maxRetries) const;
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;

    // Component References
    ILogger& logger_;
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    LGFX& display_;
    DisplayDriver& displayDriver_;
    UIController& screenManager_;
    NetworkManager& networkManager_;
    HttpDownloader& httpDownloader_;
    NtpService& ntpService_;
    TaskManager& taskManager_;
    HttpServer& httpServer_;

    // State Machine
    State currentState_;
};

#endif  // SYSTEM_INIT_H