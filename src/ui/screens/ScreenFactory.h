#pragma once

#include "IScreen.h"
#include "ui/screens/ScreenTypes.h"

class ILogger;
class DisplayManager;
class PcMetrics;
class UIController;

class ScreenFactory {
   public:
    static std::unique_ptr<IScreen> createScreen(ScreenName name, ILogger& logger,
                                                 DisplayManager* display,
                                                 PcMetrics& metrics,
                                                 UIController* controller);
};