#include "MainScreen.h"

MainScreen::MainScreen(ILogger& logger, PcMetrics& pcMetrics, UIController* uiController)
    : BaseWidgetScreen(logger, uiController), pcMetrics_(pcMetrics) {}

void MainScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("<", {0, 272, 48, 48}, 0, EventType::SHOW_SETTINGS,
                         [this](EventType action) { this->handleAction(action); })));
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(
        new ButtonWidget("Brightness", {0, 0, 88, 48}, 0, EventType::CYCLE_BRIGHTNESS,
                         [this](EventType action) { this->handleAction(action); })));
}