#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "services/PcMetrics.h"
#include "ui/ScreenTypes.h"

class UIController;  // Forward declaration

class SystemState {
   public:
    struct CoreState {
        bool isInitialized = false;
        bool isTimeSynced = false;

        // Service sync timestamps
        unsigned long nextSync_pcMetrics = 0;
        // unsigned long nextSync_Weather = 0;
    };

    struct ScreenState {
        bool isInitialized = false;
        ScreenName activeScreen = ScreenName::UNSET;
    };

    // Non-static members
    CoreState core;
    ScreenState screen;
    PcMetrics pcMetrics;

    SystemState() : core(), screen() {}

   private:
};

#endif