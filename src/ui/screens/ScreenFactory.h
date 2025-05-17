#pragma once

#include "ScreenInterface.h"
#include "ui/screens/ScreenTypes.h"

class LoggerInterface;
class DisplayManager;
class PcMetrics;
class UIController;

class ScreenFactory {
   public:
    static std::unique_ptr<ScreenInterface> createScreen(ScreenName name, LoggerInterface& logger,
                                                 DisplayManager* display,
                                                 PcMetrics& metrics,
                                                 UIController* controller);
};