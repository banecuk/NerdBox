#pragma once

#include <esp_task_wdt.h>

#include <functional>
#include <vector>

#include "core/IInitializationTarget.h"

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

    explicit InitializationStateMachine(IInitializationTarget& target);
    ~InitializationStateMachine() = default;

    // Delete copy/move operations
    InitializationStateMachine(const InitializationStateMachine&) = delete;
    InitializationStateMachine& operator=(const InitializationStateMachine&) = delete;
    InitializationStateMachine(InitializationStateMachine&&) = delete;
    InitializationStateMachine& operator=(InitializationStateMachine&&) = delete;

    bool initialize();
    bool isTerminalState() const;
    State getCurrentState() const { return currentState_; }
    const char* getStateName(State state) const;

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
    void logRetryAttempt(const char* component, uint8_t attempt, uint8_t maxRetries) const;
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;

    IInitializationTarget& target_;
    State currentState_;

    static constexpr const char* STATE_NAMES_[] = {"INITIAL",      "DISPLAY_INIT", "TASKS_INIT",
                                                   "NETWORK_INIT", "TIME_INIT",    "WATCHDOG_INIT",
                                                   "FINAL_SETUP",  "COMPLETE",     "FAILED"};
};
