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
        unsigned long nextSync_HardwareMonitor = 0;
        // unsigned long nextSync_Weather = 0;
        // unsigned long nextSync_NetworkStatus = 0;
    };

    struct ScreenState {
        bool isInitialized = false;
        ScreenName activeScreen = ScreenName::UNSET;
    };

    // Non-static members
    CoreState core;
    ScreenState screen;
    PcMetrics hmData;

    SystemState() : core(), screen() {}

   private:
};

#endif