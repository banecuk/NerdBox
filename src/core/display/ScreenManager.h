#pragma once

#include "core/display/ScreenTypes.h"
#include "core/system/SystemState.h"
#include "core/utils/Logger.h"
#include "screens/BootScreen.h"
#include "screens/IScreen.h"
#include "screens/MainScreen.h"
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
