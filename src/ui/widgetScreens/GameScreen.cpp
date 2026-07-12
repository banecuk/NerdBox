#include "GameScreen.h"

GameScreen::GameScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), pcMetrics_(pcMetrics) {}

void GameScreen::createWidgets() {
    // Big FPS number + sparkline
    widgetManager_.addWidget(std::unique_ptr<GameFpsWidget>(new GameFpsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 0, 480, 130}, 250,
        pcMetrics_)));

    // CPU/GPU/RAM/VRAM/fan tile grid
    auto metricsWidget = std::unique_ptr<GameMetricsWidget>(new GameMetricsWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 130, 480, 90}, 100,
        pcMetrics_));
    metricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(metricsWidget));

    // CPU + GPU load history strip
    widgetManager_.addWidget(std::unique_ptr<LoadHistoryWidget>(new LoadHistoryWidget(
        uiController_->getDisplayContext(), WidgetInterface::Dimensions{0, 222, 480, 46}, 250,
        pcMetrics_)));

    // Back button — flush with the left screen edge (x=0), same 272..320
    // band as MainScreen's settings button (center 296).
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<", WidgetInterface::Dimensions{0, 272, 48, 48}, 0,
        EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); }, TFT_BLACK,
        TFT_WHITE)));

    // Clock — same position/colors as the main screen, centered on the same
    // band center (296).
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(new ClockWidget(
        WidgetInterface::Dimensions{328, 276, 150, 40}, 1000, TFT_LIGHTGREY, TFT_BLACK)));
}
