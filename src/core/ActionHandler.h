#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "../display/DisplayManager.h"
#include "../display/ScreenManager.h"
#include "../utils/Logger.h"
#include "EventBus.h"

class ScreenManager;

class ActionHandler {
   public:
    ActionHandler(ScreenManager* screenManager, ILogger& logger,
                  DisplayManager* displayManager);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();

   private:
    ScreenManager* screenManager;
    DisplayManager* displayManager_;
    ILogger& logger_;
};

#endif  // ACTION_HANDLER_H