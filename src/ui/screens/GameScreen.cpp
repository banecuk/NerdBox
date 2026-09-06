#include "GameScreen.h"

#include "ui/core/Layout.h"

GameScreen::GameScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), pcMetrics_(pcMetrics) {}

void GameScreen::createWidgets() {
    // Big FPS number + sparkline
    widgetManager_.addWidget(
        std::make_unique<GameFpsWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 130}, 250, pcMetrics_),
        "game_fps");

    // CPU/GPU/RAM/VRAM/fan tile grid — 84px tall (28px rows), matching
    // MainScreen's PcMetricsWidget so both screens use the same row height.
    auto metricsWidget = std::make_unique<PcMetricsWidget>(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 130, Layout::kScreenW, 84}, 100, pcMetrics_);
    metricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(metricsWidget), "pc_metrics");

    // CPU + GPU load history strip — grown from 46 to 52px to absorb the 6px
    // freed by the tile grid's height cut above; keeps the same 2px gap above
    // it and the same bottom edge (268).
    widgetManager_.addWidget(
        std::make_unique<LoadHistoryWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 216, Layout::kScreenW, 52}, 250, pcMetrics_),
        "load_history");

    addBackButton();
    addBottomClock();
}
