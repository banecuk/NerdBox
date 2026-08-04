#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppSettings.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/DiskInfoWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Disk info screen: per-drive free space, live read/write rates. Entered by
// tapping the disk-drive tiles on the main screen; back button bottom-left.
class DiskScreen : public BaseWidgetScreen {
 public:
    DiskScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
               const AppSettings& config);
    ~DiskScreen() override = default;

 private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
};
