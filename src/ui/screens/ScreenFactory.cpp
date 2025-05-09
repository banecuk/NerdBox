#include "ScreenFactory.h"
#include "BootScreen.h"
#include "MainScreen.h"
#include "SettingsScreen.h"

std::unique_ptr<IScreen> ScreenFactory::createScreen(ScreenName name, ILogger& logger,
                                                    DisplayDriver* display,
                                                    PcMetrics& metrics,
                                                    UIController* controller) {
    switch (name) {
        case ScreenName::BOOT:
            return std::make_unique<BootScreen>(logger, display->getDisplay());
        case ScreenName::MAIN:
            return std::make_unique<MainScreen>(logger, metrics, controller);
        case ScreenName::SETTINGS:
            return std::make_unique<SettingsScreen>(logger, controller);
        default:
            return nullptr;
    }
}