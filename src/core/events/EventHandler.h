#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UIController;
class DisplayManager;

class EventHandler {
   public:
    EventHandler(UIController* uiController, ILogger& logger);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();

   private:
    UIController* uiController_;
    ILogger& logger_;
};

#endif