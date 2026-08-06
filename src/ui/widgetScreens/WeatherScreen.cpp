#include "WeatherScreen.h"

WeatherScreen::WeatherScreen(LoggerInterface& logger, UiController* uiController,
                             const AppSettings& config, WeatherData& weatherData)
    : BaseWidgetScreen(logger, uiController, config), weatherData_(weatherData) {}

void WeatherScreen::createWidgets() {
    // Daily forecast strip — covers everything above the bottom back-button
    // band (0..272). The whole strip is dynamic (column count depends on the
    // fetched dayCount), so the widget draws everything itself.
    widgetManager_.addWidget(std::unique_ptr<WeatherWidget>(new WeatherWidget(
        WidgetInterface::Dimensions{0, 0, 480, 272}, 1000, weatherData_, config_)));

    // Back button — flush with the left screen edge (x=0), same 272..320
    // band as the GameScreen back button (center 296).
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<", WidgetInterface::Dimensions{0, 272, 48, 48}, 0,
        EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); }, TFT_BLACK,
        TFT_WHITE)));
}