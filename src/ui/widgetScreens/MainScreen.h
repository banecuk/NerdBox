#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppConfigInterface.h"
#include "services/airQuality/AirQualityData.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/display/AirQualityWidget.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/FpsWidget.h"
#include "ui/widgets/display/PcMetricsWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

class MainScreen : public BaseWidgetScreen {
 public:
    MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
               AppConfigInterface& config, ApplicationMetrics& systemMetrics,
               const AirQualityData& airQualityData);
    ~MainScreen() override = default;

 private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
    ApplicationMetrics& systemMetrics_;
    const AirQualityData& airQualityData_;
};