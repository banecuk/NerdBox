#pragma once

#include "display/ScreenTypes.h"
#include "core/system/SystemState.h"
#include "utils/Logger.h"
#include "display/screens/BootScreen.h"
#include "display/screens/IScreen.h"
#include "display/screens/MainScreen.h"
#include "services/PcMetrics.h"

// Forward declaration
class MainScreen;

class ScreenManager {
   public:
    explicit ScreenManager(ILogger& logger, LGFX* lcd, PcMetrics& hmData,
                           SystemState::ScreenState& screenState);

    void initialize();
    bool setScreen(ScreenName screenName);
    void draw();
    void handleTouchInput();

    // Methods for button actions
    void resetDevice();
    void cycleBrightness();

   private:
    ILogger& logger_;
    LGFX* lcd_;
    PcMetrics& pcMetrics_;

    IScreen* currentScreen_;

    BootScreen bootScreen_;
    MainScreen* mainScreen_;

    SystemState::ScreenState& screenState_;
};

#include "screens/MainScreen.h"
