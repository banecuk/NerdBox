#include "MainScreen.h"

MainScreen::MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics,
                       UIController* uiController)
    : BaseWidgetScreen(logger, uiController), pcMetrics_(pcMetrics) {}

void MainScreen::createWidgets() {
    widgetManager_.addWidget(std::unique_ptr<PcMetricsWidget>(new PcMetricsWidget(
        uiController_->getDisplayContext(), {0, 0, 480, 180}, 100, pcMetrics_)));

    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget({328, 288, 150, 24}, 1000, TFT_LIGHTGREY, TFT_BLACK, 3)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        "<", {0, 272, 48, 48}, 0, EventType::SHOW_SETTINGS,
        [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)));
}