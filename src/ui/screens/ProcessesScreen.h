#pragma once

#include "config/AppSettings.h"
#include "services/processes/ProcessData.h"
#include "ui/screens/base/BaseWidgetScreen.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/ProcessListWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Top-8-by-CPU / top-8-by-RAM / top-8-by-disk process list screen. Entered
// by tapping the "Processes" button in CpuClockScreen's footer; back button
// returns to CPU_CLOCK (not MAIN, as requested). Fetches its data via
// ProcessStreamJob only while this screen is active — see
// docs-local/PROCESSES-SCREEN-PLAN.md.
class ProcessesScreen : public BaseWidgetScreen {
 public:
    ProcessesScreen(LoggerInterface& logger, ProcessData& processData, UiController* uiController,
                    const AppSettings& config);
    ~ProcessesScreen() override = default;

 private:
    void createWidgets() override;
    ProcessData& processData_;
};
