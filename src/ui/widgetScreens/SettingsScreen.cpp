#include "SettingsScreen.h"

SettingsScreen::SettingsScreen(LoggerInterface& logger, UIController* uiController)
    : BaseWidgetScreen(logger, uiController) {}

void SettingsScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288 - 24, 150, 24}, 1000, TFT_YELLOW, TFT_BLACK, 3)));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {0, 320 - 1 - 48, 48, 48}, 0, EventType::SHOW_MAIN,
                         [this](EventType action) { this->handleAction(action); })));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Reset", {480-20-100, 20, 100, 48}, 0, EventType::RESET_DEVICE,
                         [this](EventType action) { this->handleAction(action); })));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Brightness", {20, 20, 100, 48}, 0, EventType::CYCLE_BRIGHTNESS,
                         [this](EventType action) { this->handleAction(action); })));
}