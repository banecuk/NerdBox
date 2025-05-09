#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UIController;
class DisplayDriver;

class ActionHandler {
   public:
    ActionHandler(UIController* uiController, ILogger& logger);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();
    void requestSettingsScreen();
    void requestMainScreen();

   private:
    UIController* uiController_;
    ILogger& logger_;
};

#endif  // ACTION_HANDLER_H