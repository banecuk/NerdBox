#pragma once

#include "BaseWidgetScreen.h"
#include "services/PcMetrics.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"
#include "ui/widgets/PcMetricsWidget.h"

class MainScreen : public BaseWidgetScreen {
   public:
    MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UIController* uiController);
    ~MainScreen() override = default;

   private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
};