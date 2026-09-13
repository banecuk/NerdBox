#include "CpuClockScreen.h"

#include "ui/core/Layout.h"
#include "ui/core/Theme.h"

CpuClockScreen::CpuClockScreen(LoggerInterface& logger, CpuClockData& cpuClockData,
                               UiController* uiController, const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), cpuClockData_(cpuClockData) {}

void CpuClockScreen::createWidgets() {
    // Per-core clock grid + bus speed — content area above the bottom band.
    widgetManager_.addWidget(
        std::make_unique<CpuClockWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, Layout::kContentH}, 250,
            cpuClockData_),
        "cpu_clock_grid");

    addBackButton();

    // Footer button — opens the process list screen; back from there returns
    // here (not MAIN).
    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            uiController_->getDisplayContext(), "Processes",
            WidgetInterface::Dimensions{56, Layout::kBottomBarY, 120, Layout::kButtonSize}, 0,
            EventType::SHOW_PROCESSES, [this](EventType a) { this->handleAction(a); },
            Theme::kSurface, TFT_WHITE),
        "processes_button");

    addBottomClock();
}
