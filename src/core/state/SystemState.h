#pragma once

#include "core/ScreenTypes.h"

class SystemState {
 public:
    struct CoreState {
        bool isInitialized = false;
        bool isTimeSynced = false;
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