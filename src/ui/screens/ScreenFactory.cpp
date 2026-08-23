#include "ScreenFactory.h"

#include "BootScreen.h"
#include "services/airQuality/AirQualityData.h"
#include "services/network/NetworkStatus.h"
#include "ui/screens/CalendarScreen.h"
#include "ui/screens/CpuClockScreen.h"
#include "ui/screens/DiskScreen.h"
#include "ui/screens/GameScreen.h"
#include "ui/screens/MainScreen.h"
#include "ui/screens/ProcessesScreen.h"
#include "ui/screens/SettingsScreen.h"
#include "ui/screens/WeatherScreen.h"

std::unique_ptr<ScreenInterface> ScreenFactory::createScreen(ScreenName name,
                                                             const ScreenCreationContext& ctx) {
    switch (name) {
        case ScreenName::BOOT:
            return std::make_unique<BootScreen>(ctx.screenLogQueue, ctx.display->getDisplay());
        case ScreenName::MAIN:
            return std::make_unique<MainScreen>(ctx.logger, ctx.metrics, ctx.controller, ctx.config,
                                                ctx.systemMetrics, ctx.airQualityData,
                                                ctx.netStatus, ctx.audioData, ctx.weatherData);
        case ScreenName::SETTINGS:
            return std::make_unique<SettingsScreen>(ctx.logger, ctx.controller, ctx.config,
                                                    ctx.networkManager, ctx.systemMetrics);
        case ScreenName::GAME:
            return std::make_unique<GameScreen>(ctx.logger, ctx.metrics, ctx.controller,
                                                ctx.config);
        case ScreenName::DISKS:
            return std::make_unique<DiskScreen>(ctx.logger, ctx.metrics, ctx.controller,
                                                ctx.config);
        case ScreenName::CPU_CLOCK:
            return std::make_unique<CpuClockScreen>(ctx.logger, ctx.cpuClockData, ctx.controller,
                                                    ctx.config);
        case ScreenName::PROCESSES:
            return std::make_unique<ProcessesScreen>(ctx.logger, ctx.processData, ctx.controller,
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
