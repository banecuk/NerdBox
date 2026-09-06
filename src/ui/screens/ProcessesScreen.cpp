#include "ProcessesScreen.h"

#include "ui/core/Layout.h"

ProcessesScreen::ProcessesScreen(LoggerInterface& logger, ProcessData& processData,
                                 UiController* uiController, const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config), processData_(processData) {}

void ProcessesScreen::createWidgets() {
    // Three-column process list — content area above the bottom band.
    widgetManager_.addWidget(
        std::make_unique<ProcessListWidget>(
            uiController_->getDisplayContext(),
            WidgetInterface::Dimensions{0, 0, Layout::kScreenW, Layout::kContentH}, 500,
            processData_),
        "process_list");

    // Back button — returns to CPU_CLOCK, not MAIN, as requested.
    addBackButton(EventType::SHOW_CPU_CLOCK);
    addBottomClock();
}
