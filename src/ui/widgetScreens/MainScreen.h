#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppConfigInterface.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/PcMetricsWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

class MainScreen : public BaseWidgetScreen {
 public:
    MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
               AppConfigInterface& config, ApplicationMetrics& systemMetrics);
    ~MainScreen() override = default;

 private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
    ApplicationMetrics& systemMetrics_;
};