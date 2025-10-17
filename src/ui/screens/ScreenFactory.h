#pragma once

#include "ScreenInterface.h"
#include "config/AppConfigInterface.h"
#include "ui/UiController.h"
#include "ui/screens/ScreenTypes.h"

class LoggerInterface;
class DisplayManager;
class PcMetrics;
class UIController;

class ScreenFactory {
   public:
    static std::unique_ptr<ScreenInterface> createScreen(
        ScreenName name, LoggerInterface& logger, DisplayManager* display,
        PcMetrics& metrics, UiController* controller, AppConfigInterface& config);
};