#pragma once

#include "config/AppSettings.h"
#include "services/cpuClock/CpuClockData.h"
#include "ui/screens/base/BaseWidgetScreen.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/CpuClockWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Per-core CPU clock speed screen. Entered by tapping ThreadsWidget on the
// main screen; back button bottom-left like every other screen. Fetches its
// data via CpuClockStreamJob only while this screen is active — see
// docs-local/CPU-CLOCK-SCREEN-PLAN.md.
class CpuClockScreen : public BaseWidgetScreen {
 public:
    CpuClockScreen(LoggerInterface& logger, CpuClockData& cpuClockData, UiController* uiController,
                   const AppSettings& config);
    ~CpuClockScreen() override = default;

 private:
    void createWidgets() override;
    CpuClockData& cpuClockData_;
};
