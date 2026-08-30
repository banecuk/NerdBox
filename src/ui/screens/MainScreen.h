#pragma once

#include "config/AppSettings.h"
#include "services/airQuality/AirQualityData.h"
#include "services/audio/AudioData.h"
#include "services/network/NetworkStatus.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/roomClimate/RoomClimateData.h"
#include "services/weather/WeatherData.h"
#include "ui/screens/base/BaseWidgetScreen.h"
#include "ui/widgets/display/AirQualityWidget.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/DiskBandWidget.h"
#include "ui/widgets/display/MultiWidget.h"
#include "ui/widgets/display/NetworkTrafficWidget.h"
#include "ui/widgets/display/NetworkWidget.h"
#include "ui/widgets/display/PcMetricsWidget.h"
#include "ui/widgets/display/RoomClimateWidget.h"
#include "ui/widgets/display/ThreadsWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

class MainScreen : public BaseWidgetScreen {
 public:
    MainScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
               const AppSettings& config, ApplicationMetrics& systemMetrics,
               const AirQualityData& airQualityData, const NetworkStatus& netStatus,
               const AudioData& audioData, WeatherData& weatherData,
               const RoomClimateData& roomClimateData);
    ~MainScreen() override = default;

 private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
    ApplicationMetrics& systemMetrics_;
    const AirQualityData& airQualityData_;
    const NetworkStatus& netStatus_;
    const AudioData& audioData_;
    WeatherData& weatherData_;
    const RoomClimateData& roomClimateData_;
};
