#include "GameScreen.h"

#include "ui/core/Layout.h"

GameScreen::GameScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), pcMetrics_(pcMetrics) {}

void GameScreen::createWidgets() {
    // Big FPS number + sparkline
    widgetManager_.addWidget(std::unique_ptr<GameFpsWidget>(new GameFpsWidget(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 130}, 250, pcMetrics_)));

    // CPU/GPU/RAM/VRAM/fan tile grid — 84px tall (28px rows), matching
    // MainScreen's PcMetricsWidget so both screens use the same row height.
    auto metricsWidget = std::unique_ptr<PcMetricsWidget>(new PcMetricsWidget(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 130, Layout::kScreenW, 84}, 100, pcMetrics_));
    metricsWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(metricsWidget));

    // CPU + GPU load history strip — grown from 46 to 52px to absorb the 6px
    // freed by the tile grid's height cut above; keeps the same 2px gap above
    // it and the same bottom edge (268).
    widgetManager_.addWidget(std::unique_ptr<LoadHistoryWidget>(new LoadHistoryWidget(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 216, Layout::kScreenW, 52}, 250, pcMetrics_)));

    // Back button — flush with the left screen edge (x=0), in the shared
    // bottom band (center matches MainScreen's settings button, offset by 3px).
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                    Layout::kButtonSize},
        0, EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));

    // Clock — same position/colors as the main screen, centered on the same
    // band center (296).
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget(WidgetInterface::Dimensions{328, 276, Layout::kClockW, 40}, 1000,
                        TFT_LIGHTGREY, TFT_BLACK)));
}
