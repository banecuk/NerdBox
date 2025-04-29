#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "EventBus.h"
#include "utils/Logger.h"

// Forward declarations
class UIController;
class DisplayDriver;

class ActionHandler {
   public:
    ActionHandler(UIController* uiController, ILogger& logger,
                  DisplayDriver* displayDriver);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();

   private:
    UIController* screenManager;
    DisplayDriver* displayDriver_;
    ILogger& logger_;
};

#endif  // ACTION_HANDLER_H