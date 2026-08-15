#pragma once

#include "ui/widgetScreens/BaseWidgetScreen.h"
#include "config/AppSettings.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/GameFpsWidget.h"
#include "ui/widgets/display/LoadHistoryWidget.h"
#include "ui/widgets/display/PcMetricsWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Gaming-focused metrics screen: large FPS + history sparkline, CPU/GPU/RAM/
// VRAM/fan tiles. Entered by tapping the FPS tile on the main screen;
// deliberately omits disk, weather, and network widgets.
class GameScreen : public BaseWidgetScreen {
 public:
    GameScreen(LoggerInterface& logger, PcMetrics& pcMetrics, UiController* uiController,
               const AppSettings& config);
    ~GameScreen() override = default;

 private:
    void createWidgets() override;
    PcMetrics& pcMetrics_;
};
