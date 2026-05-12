#include "ScreenFactory.h"

#include "BootScreen.h"
#include "ui/widgetScreens/MainScreen.h"
#include "ui/widgetScreens/SettingsScreen.h"

std::unique_ptr<ScreenInterface> ScreenFactory::createScreen(ScreenName name,
                                                             const ScreenCreationContext& ctx) {
    switch (name) {
        case ScreenName::BOOT:
            return std::make_unique<BootScreen>(ctx.logger, ctx.display->getDisplay());
        case ScreenName::MAIN:
            return std::make_unique<MainScreen>(ctx.logger, ctx.metrics, ctx.controller,
                                                ctx.config, ctx.systemMetrics);
        case ScreenName::SETTINGS:
            return std::make_unique<SettingsScreen>(ctx.logger, ctx.controller, ctx.config,
                                                    ctx.networkManager);
        default:
            return nullptr;
    }
}
