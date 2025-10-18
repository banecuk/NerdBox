#pragma once

#include "EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UiController;
class DisplayManager;

class EventHandler {
 public:
    EventHandler(UiController* uiController, LoggerInterface& logger);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();

 private:
    UiController* uiController_;
    LoggerInterface& logger_;
};