#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "../display/ScreenManager.h"
#include "EventBus.h"
#include "../utils/Logger.h"

class ScreenManager;

class ActionHandler {
   public:
    ActionHandler(ScreenManager* screenManager, ILogger& logger, LGFX* lcd);

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();

   private:
    ScreenManager* screenManager;
    ILogger& logger_;
    LGFX* lcd_;
};

#endif  // ACTION_HANDLER_H