#pragma once

#include "config/AppConfigInterface.h"
#include "ScreenInterface.h"
#include "ui/screens/ScreenTypes.h"
#include "ui/UiController.h"

class LoggerInterface;
class DisplayManager;
class PcMetrics;
class UIController;

class ScreenFactory {
 public:
    static std::unique_ptr<ScreenInterface>
    createScreen(ScreenName name, LoggerInterface& logger, DisplayManager* display,
                 PcMetrics& metrics, UiController* controller, AppConfigInterface& config);
};