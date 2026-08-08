#pragma once

#include <esp_task_wdt.h>

#include "core/IInitializationTarget.h"

class InitializationStateMachine {
 public:
    enum class State {
        INITIAL,
        DISPLAY_INIT,
        WATCHDOG_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        FINAL_SETUP,
        COMPLETE,
        FAILED
    };

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
    uint16_t calculateBackoffDelay(uint8_t attempt, uint16_t baseDelay) const;

    // Adds the calling (main/setup) task to the watchdog. Deliberately called
    // only after NETWORK_INIT/TIME_INIT finish their blocking work (WiFi
    // connect + up to kDefaultTimeSyncRetries NTP attempts, worst case ~13s+)
    // — nothing resets the watchdog for this task until Application::run()
    // starts, so joining it any earlier risks a watchdog panic mid-boot.
    void addMainTaskToWatchdog();

    IInitializationTarget& target_;
    State currentState_;

    static constexpr const char* STATE_NAMES_[] = {"INITIAL",     "DISPLAY_INIT", "WATCHDOG_INIT",
                                                   "TASKS_INIT",  "NETWORK_INIT", "TIME_INIT",
                                                   "FINAL_SETUP", "COMPLETE",     "FAILED"};
};
