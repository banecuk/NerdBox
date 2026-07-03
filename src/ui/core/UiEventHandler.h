#pragma once

#include "core/events/EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UiController;
class DisplayManager;

// UiEventHandler — translates EventBus events into UiController / DisplayManager actions.
//
// Subscription table (registered in registerHandlers()):
//   EventType::NONE            → log only (debug aid)
//   EventType::RESET_DEVICE    → resetDevice()      — shows message, calls ESP.restart()
//   EventType::CYCLE_BRIGHTNESS → cycleBrightness() — steps through brightness levels & saves to NVS
//   EventType::SHOW_SETTINGS   → requestSettingsScreen()
//   EventType::SHOW_MAIN       → requestMainScreen()
class UiEventHandler {
 public:
    UiEventHandler(UiController* uiController, LoggerInterface& logger);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();

 private:
    UiController* uiController_;
    LoggerInterface& logger_;
};
