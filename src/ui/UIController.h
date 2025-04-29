#pragma once

#include <memory>  // For std::unique_ptr

#include "core/system/SystemState.h"
#include "services/PcMetrics.h"
#include "ui/DisplayDriver.h"
#include "ui/ScreenTypes.h"
#include "ui/screens/IScreen.h"
#include "utils/Logger.h"

// Forward declarations
class BootScreen;
class MainScreen;
class ActionHandler;

class UIController {
   public:
    explicit UIController(ILogger& logger, DisplayDriver* displayDriver,
                          PcMetrics& hmData, SystemState::ScreenState& screenState);
    ~UIController();

    void initialize();
    bool setScreen(ScreenName screenName);
    void draw();
    void handleTouchInput();

   private:
    void clearDisplay();

    ILogger& logger_;
    DisplayDriver* displayDriver_;
    PcMetrics& pcMetrics_;
    SystemState::ScreenState& screenState_;

    std::unique_ptr<IScreen> currentScreen_;
    std::unique_ptr<BootScreen> bootScreen_;
    std::unique_ptr<MainScreen> mainScreen_;
    std::unique_ptr<ActionHandler> actionHandler_;
};