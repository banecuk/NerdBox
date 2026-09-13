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

    // CPU + GPU load history strip — starts flush against PcMetricsWidget's
    // bottom edge (214) and ends flush against the bottom band (previously a
    // 2px gap above and a 4px gap below — accidental seams, see N2) so the
    // three widgets tile the screen with zero seams between them.
    static constexpr uint16_t kLoadHistoryY = 214;
    static constexpr uint16_t kLoadHistoryH = Layout::kBottomBarY - kLoadHistoryY;
    widgetManager_.addWidget(
        std::make_unique<LoadHistoryWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, kLoadHistoryY, Layout::kScreenW, kLoadHistoryH}, 250,
            pcMetrics_),
        "load_history");

    addBackButton();
    addBottomClock();
}
