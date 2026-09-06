#include "WeatherScreen.h"

#include "ui/core/Layout.h"

WeatherScreen::WeatherScreen(LoggerInterface& logger, UiController* uiController,
                             const AppSettings& config, WeatherData& weatherData)
    : BaseWidgetScreen(logger, uiController, config), weatherData_(weatherData) {}

void WeatherScreen::createWidgets() {
    // Daily forecast strip — covers everything above the bottom back-button
    // band. The whole strip is dynamic (column count depends on the fetched
    // dayCount), so the widget draws everything itself.
    widgetManager_.addWidget(
        std::make_unique<WeatherWidget>(
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, Layout::kContentH}, 1000,
            weatherData_, config_),
        "weather");

    addBackButton();
}