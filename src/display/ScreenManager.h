#pragma once

#include <memory> // For std::unique_ptr
#include "core/system/SystemState.h"
#include "display/DisplayManager.h"
#include "display/ScreenTypes.h"
#include "display/screens/IScreen.h"
#include "services/PcMetrics.h"
#include "utils/Logger.h"

// Forward declarations
class BootScreen;
class MainScreen;
class ActionHandler;

class ScreenManager {
public:
    explicit ScreenManager(ILogger& logger, DisplayManager* displayManager,
                         PcMetrics& hmData, SystemState::ScreenState& screenState);
    ~ScreenManager();

    void initialize();
    bool setScreen(ScreenName screenName);
    void draw();
    void handleTouchInput();

private:
    void clearDisplay();

    ILogger& logger_;
    DisplayManager* displayManager_;
    PcMetrics& pcMetrics_;
    SystemState::ScreenState& screenState_;

    std::unique_ptr<IScreen> currentScreen_;
    std::unique_ptr<BootScreen> bootScreen_;
    std::unique_ptr<MainScreen> mainScreen_;
    std::unique_ptr<ActionHandler> actionHandler_;
};