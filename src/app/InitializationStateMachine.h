#pragma once

#include <cstddef>
#include <cstdint>

#include "config/AppSettings.h"
#include "core/IInitializationTarget.h"

// Boot sequence: a fixed, ordered table of steps run once from setup().
//
// Each step returns true to continue or false for a boot-fatal failure, in
// which case initialize() stops, records FAILED and returns false (main.cpp
// then reboots). Non-fatal problems (no WiFi, no NTP, watchdog couldn't be
// armed) are logged by the step itself and the sequence carries on.
//
// No Arduino/ESP-IDF calls live here — every platform operation goes through
// IInitializationTarget — so the sequencing, failure handling and the
// time-sync backoff are host-tested (test/InitializationStateMachineTest.cpp).
class InitializationStateMachine {
 public:
    // One value per step, in boot order, plus the pre-run and terminal states.
    // The step values index kSteps[] directly.
    enum class State : uint8_t {
        DISPLAY_INIT,
        WATCHDOG_INIT,
        TASKS_INIT,
        NETWORK_INIT,
        TIME_INIT,
        FINAL_SETUP,
        INITIAL,
        COMPLETE,
        FAILED
    };

    InitializationStateMachine(IInitializationTarget& target, const AppSettings& config);
    ~InitializationStateMachine() = default;

    // Delete copy/move operations
    InitializationStateMachine(const InitializationStateMachine&) = delete;
    InitializationStateMachine& operator=(const InitializationStateMachine&) = delete;
    InitializationStateMachine(InitializationStateMachine&&) = delete;
    InitializationStateMachine& operator=(InitializationStateMachine&&) = delete;

    // Runs every step in order. Returns false on the first boot-fatal step
    // failure. Only meant to be called once; a second call returns the first
    // call's result without re-running anything.
    bool initialize();

    State getCurrentState() const { return currentState_; }
    // Percentage of the boot sequence reached (the current step's entry in
    // kSteps[]; 100 once COMPLETE). No on-screen consumer since the boot
    // progress bar was dropped (B39) — kept so a future one doesn't have to
    // re-derive it from the step order.
    uint8_t progressPercent() const;
    static const char* getStateName(State state);

    // Exponential backoff with jitter for time-sync retry `attempt` (1-based).
    // Computed in 64-bit, clamped last, so a large base can never narrow or
    // wrap to a shorter delay (B16, B62). `jitter` is the caller's random
    // value, already in [0, kBackoffJitterMs).
    static uint32_t backoffDelayMs(uint8_t attempt, uint32_t baseDelayMs, uint32_t jitter);
    static constexpr uint32_t kMaxBackoffDelayMs = 30000;

 private:
    using StepFn = bool (InitializationStateMachine::*)();

    struct Step {
        State state;
        const char* name;
        uint8_t progressPercent;  // reached when this step *starts*
        StepFn run;
    };

    bool stepDisplayInit();
    bool stepWatchdogInit();
    bool stepTasksInit();
    bool stepNetworkInit();
    bool stepTimeInit();
    bool stepFinalSetup();

    // Adds the calling (main/setup) task to the watchdog. Deliberately called
    // only after NETWORK_INIT/TIME_INIT finish their blocking work (WiFi
    // connect + up to kDefaultTimeSyncRetries NTP attempts, worst case ~13s+)
    // — nothing resets the watchdog for this task until Application::run()
    // starts, so joining it any earlier risks a watchdog panic mid-boot.
    void addMainTaskToWatchdog();

    void transitionTo(State newState);

    static constexpr Step kSteps[] = {
        {State::DISPLAY_INIT, "DISPLAY_INIT", 0, &InitializationStateMachine::stepDisplayInit},
        {State::WATCHDOG_INIT, "WATCHDOG_INIT", 15, &InitializationStateMachine::stepWatchdogInit},
        {State::TASKS_INIT, "TASKS_INIT", 25, &InitializationStateMachine::stepTasksInit},
        {State::NETWORK_INIT, "NETWORK_INIT", 40, &InitializationStateMachine::stepNetworkInit},
        {State::TIME_INIT, "TIME_INIT", 70, &InitializationStateMachine::stepTimeInit},
        {State::FINAL_SETUP, "FINAL_SETUP", 90, &InitializationStateMachine::stepFinalSetup},
    };
    static constexpr size_t kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

    static constexpr bool stepsAreOrdered() {
        for (size_t i = 0; i < kStepCount; ++i) {
            if (static_cast<size_t>(kSteps[i].state) != i)
                return false;
            if (i > 0 && kSteps[i].progressPercent <= kSteps[i - 1].progressPercent)
                return false;
            if (kSteps[i].progressPercent >= 100)
                return false;
        }
        return true;
    }

    IInitializationTarget& target_;
    const AppSettings& config_;
    State currentState_ = State::INITIAL;
};
