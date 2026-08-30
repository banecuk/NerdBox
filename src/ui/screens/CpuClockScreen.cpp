#include "CpuClockScreen.h"

#include "ui/core/Layout.h"

CpuClockScreen::CpuClockScreen(LoggerInterface& logger, CpuClockData& cpuClockData,
                               UiController* uiController, const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), cpuClockData_(cpuClockData) {}

void CpuClockScreen::createWidgets() {
    // Per-core clock grid + bus speed — content area above the bottom band.
    widgetManager_.addWidget(
        std::unique_ptr<CpuClockWidget>(new CpuClockWidget(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 272}, 250, cpuClockData_)),
        "cpu_clock_grid");

    // Back button — same position/style as every other screen.
    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            uiController_->getDisplayContext(), "<",
            WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                        Layout::kButtonSize},
            0, EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); },
            TFT_BLACK, TFT_WHITE)),
        "back_button");

    // Footer button — opens the process list screen; back from there returns
    // here (not MAIN).
    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            uiController_->getDisplayContext(), "Processes",
            WidgetInterface::Dimensions{56, Layout::kBottomBarY, 120, Layout::kButtonSize},
            0, EventType::SHOW_PROCESSES, [this](EventType a) { this->handleAction(a); },
            TFT_BLACK, TFT_WHITE)),
        "processes_button");

    // Clock — same position/colors as the disk screen.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
                                  new ClockWidget(WidgetInterface::Dimensions{328, 276,
                                                                               Layout::kClockW, 40},
                                                  1000, TFT_LIGHTGREY, TFT_BLACK)),
                              "clock");
}
