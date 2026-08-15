#include "ScreenFactory.h"

#include "BootScreen.h"
#include "services/airQuality/AirQualityData.h"
#include "services/network/NetworkStatus.h"
#include "ui/widgetScreens/CalendarScreen.h"
#include "ui/widgetScreens/DiskScreen.h"
#include "ui/widgetScreens/GameScreen.h"
#include "ui/widgetScreens/MainScreen.h"
#include "ui/widgetScreens/SettingsScreen.h"
#include "ui/widgetScreens/WeatherScreen.h"

std::unique_ptr<ScreenInterface> ScreenFactory::createScreen(ScreenName name,
                                                             const ScreenCreationContext& ctx) {
    switch (name) {
        case ScreenName::BOOT:
            return std::make_unique<BootScreen>(ctx.logger, ctx.display->getDisplay());
        case ScreenName::MAIN:
            return std::make_unique<MainScreen>(ctx.logger, ctx.metrics, ctx.controller, ctx.config,
                                                ctx.systemMetrics, ctx.airQualityData,
                                                ctx.netStatus);
        case ScreenName::SETTINGS:
            return std::make_unique<SettingsScreen>(ctx.logger, ctx.controller, ctx.config,
                                                    ctx.networkManager, ctx.systemMetrics);
        case ScreenName::GAME:
            return std::make_unique<GameScreen>(ctx.logger, ctx.metrics, ctx.controller,
                                                ctx.config);
        case ScreenName::DISKS:
            return std::make_unique<DiskScreen>(ctx.logger, ctx.metrics, ctx.controller,
                                                ctx.config);
        case ScreenName::WEATHER:
            return std::make_unique<WeatherScreen>(ctx.logger, ctx.controller, ctx.config,
                                                   ctx.weatherData);
        case ScreenName::CALENDAR:
            return std::make_unique<CalendarScreen>(ctx.logger, ctx.controller, ctx.config);
        default:
            return nullptr;
    }
}
