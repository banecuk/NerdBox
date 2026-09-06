#include "DiskScreen.h"

#include "ui/core/Layout.h"

DiskScreen::DiskScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
                       const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), pcMetrics_(pcMetrics) {}

void DiskScreen::createWidgets() {
    // Per-drive rows — content area above the bottom band.
    auto diskWidget = std::make_unique<DiskInfoWidget>(
        uiController_->getDisplayContext(),
        WidgetInterface::Dimensions{0, 0, Layout::kScreenW, Layout::kContentH}, 250, pcMetrics_);
    diskWidget->setStaleTimeout(5000);
    widgetManager_.addWidget(std::move(diskWidget), "disk_info");

    addBackButton();
    addBottomClock();
}
