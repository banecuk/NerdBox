#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(LoggerInterface& logger, UiController* uiController,
                               AppConfigInterface& config)
    : BaseWidgetScreen(logger, uiController, config) {}

void SettingsScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 288, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK, 3)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));

    // menu buttons
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "Reset",
        WidgetInterface::Dimensions{480 - 100, 0, 100, 48}, 0, EventType::RESET_DEVICE,
        [this](EventType action) { this->handleAction(action); }, TFT_RED, TFT_WHITE)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget(uiController_->getDisplayContext(), "Brightness",
                         WidgetInterface::Dimensions{0, 0, 100, 48}, 0, EventType::CYCLE_BRIGHTNESS,
                         [this](EventType action) { this->handleAction(action); })));
}