#pragma once

#include "core/ScreenTypes.h"

class SystemState {
 public:
    struct CoreState {
        bool isInitialized = false;
        bool isTimeSynced = false;
        // Boot progress, 0-100 — written by InitializationStateMachine::
        // transitionTo() (via IInitializationTarget::setBootProgressPercent()),
        // read by BootScreen to drive its progress bar (see
        // docs-local/03-visual-ux.md V10). Plain uint8_t, not atomic<>: same
        // single-writer/single-reader convention as the two bools above.
        uint8_t bootProgressPercent = 0;
    };

    struct ScreenState {
        bool isInitialized = false;
        ScreenName activeScreen = ScreenName::NONE;
    };

    // Non-static members
    CoreState core;
    ScreenState screen;

    SystemState() : core(), screen() {}

 private:
};