#pragma once

#include "EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UIController;
class DisplayManager;

class EventHandler {
   public:
    EventHandler(UIController* uiController, LoggerInterface& logger);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();

   private:
    UIController* uiController_;
    LoggerInterface& logger_;
};