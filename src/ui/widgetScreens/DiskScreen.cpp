#include "DiskScreen.h"

#include "ui/core/Layout.h"

DiskScreen::DiskScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), pcMetrics_(pcMetrics) {}

void DiskScreen::createWidgets() {
    // Per-drive rows — content area above the bottom band.
    auto diskWidget = std::unique_ptr<DiskInfoWidget>(new DiskInfoWidget(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 272}, 250, pcMetrics_));
    diskWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(diskWidget));

    // Back button — same position/style as GameScreen's back button: flush
    // with the left screen edge (x=0), same shared bottom band.
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                    Layout::kButtonSize},
        0, EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));

    // Clock — same position/colors as the main screen.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget(WidgetInterface::Dimensions{328, 276, Layout::kClockW, 40}, 1000,
                        TFT_LIGHTGREY, TFT_BLACK)));
}
