#pragma once

#include "IScreen.h"
#include "ui/ScreenTypes.h"

class ILogger;
class DisplayDriver;
class PcMetrics;
class UIController;

class ScreenFactory {
public:
    static std::unique_ptr<IScreen> createScreen(ScreenName name, ILogger& logger,
                                                DisplayDriver* display,
                                                PcMetrics& metrics,
                                                UIController* controller);
};