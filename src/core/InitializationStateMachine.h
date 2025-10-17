#pragma once

#include <esp_task_wdt.h>  // Add this include

#include <functional>
#include <vector>

#include "config/AppConfigService.h"
#include "utils/Logger.h"

class ApplicationComponents;

class InitializationStateMachine {
   public:
    enum class State {
        INITIAL,
        DISPLAY_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        WATCHDOG_INIT,
        FINAL_SETUP,
        COMPLETE,
        FAILED
    };

    using StateHandler = std::function<bool()>;

    InitializationStateMachine(ApplicationComponents& components, Logger& logger);
    ~InitializationStateMachine() = default;

    // Delete copy/move operations
    InitializationStateMachine(const InitializationStateMachine&) = delete;
    InitializationStateMachine& operator=(const InitializationStateMachine&) = delete;
    InitializationStateMachine(InitializationStateMachine&&) = delete;
    InitializationStateMachine& operator=(InitializationStateMachine&&) = delete;

    bool initialize();
    bool isTerminalState() const;
    State getCurrentState() const { return currentState_; }
    String getStateName(State state) const;

   private:
    // State handlers
    bool handleInitial();
    bool handleDisplayInit();
    bool handleTasksInit();
    bool handleNetworkInit();
    bool handleTimeInit();
    bool handleWatchdogInit();
    bool handleFinalSetup();
    bool handleComplete();
    bool handleFailed();

    // Helper methods
    void transitionTo(State newState);
    void logRetryAttempt(const char* component, uint8_t attempt,
                         uint8_t maxRetries) const;
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;

    // Components
    ApplicationComponents& components_;
    Logger& logger_;

    // State
    State currentState_;

    static constexpr const char* STATE_NAMES_[] = {
        "INITIAL",       "DISPLAY_INIT", "TASKS_INIT", "NETWORK_INIT", "TIME_INIT",
        "WATCHDOG_INIT", "FINAL_SETUP",  "COMPLETE",   "FAILED"};
};