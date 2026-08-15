#pragma once

#include "config/AppSettings.h"
#include "services/weather/WeatherData.h"
#include "ui/screens/base/BaseWidgetScreen.h"
#include "ui/widgets/display/WeatherWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Weather forecast screen: full-width daily column strip (up to 7 days) plus
// a back button. Entered by tapping the AirQualityWidget on the main screen.
// Data is refreshed by WeatherJob every ~2h regardless of active screen,
// plus immediately on entry to this screen (UiController::loadAndActivateScreen).
class WeatherScreen : public BaseWidgetScreen {
 public:
    WeatherScreen(LoggerInterface& logger, UiController* uiController, const AppSettings& config,
                  WeatherData& weatherData);
    ~WeatherScreen() override = default;

 private:
    void createWidgets() override;
    WeatherData& weatherData_;
};