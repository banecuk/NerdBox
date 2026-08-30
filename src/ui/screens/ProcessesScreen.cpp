#include "ProcessesScreen.h"

#include "ui/core/Layout.h"

ProcessesScreen::ProcessesScreen(LoggerInterface& logger, ProcessData& processData,
                                 UiController* uiController, const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), processData_(processData) {}

void ProcessesScreen::createWidgets() {
    // Three-column process list — content area above the bottom band.
    widgetManager_.addWidget(
        std::unique_ptr<ProcessListWidget>(new ProcessListWidget(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 272}, 500, processData_)),
        "process_list");

    // Back button — returns to CPU_CLOCK, not MAIN, as requested.
    widgetManager_.addWidget(
        std::unique_ptr<ButtonWidget>(new ButtonWidget(
            uiController_->getDisplayContext(), "<",
            WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                        Layout::kButtonSize},
            0, EventType::SHOW_CPU_CLOCK,
            [this](EventType action) { this->handleAction(action); }, TFT_BLACK, TFT_WHITE)),
        "back_button");

    // Clock — same position/colors as the CPU-clock and disk screens.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
                                  new ClockWidget(WidgetInterface::Dimensions{328, 276,
                                                                               Layout::kClockW, 40},
                                                  1000, TFT_LIGHTGREY, TFT_BLACK)),
                              "clock");
}
