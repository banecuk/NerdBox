#include "ScreenFactory.h"

#include "BootScreen.h"
#include "ui/widgetScreens/MainScreen.h"
#include "ui/widgetScreens/SettingsScreen.h"

std::unique_ptr<ScreenInterface> ScreenFactory::createScreen(ScreenName name, LoggerInterface& logger,
                                                     DisplayManager* display,
                                                     PcMetrics& metrics,
                                                     UIController* controller,
                                                     AppConfigInterface& config) {
    switch (name) {
        case ScreenName::BOOT:
            return std::make_unique<BootScreen>(logger, display->getDisplay());
        case ScreenName::MAIN:
            return std::make_unique<MainScreen>(logger, metrics, controller, config);
        case ScreenName::SETTINGS:
            return std::make_unique<SettingsScreen>(logger, controller, config);
        default:
            return nullptr;
    }
}