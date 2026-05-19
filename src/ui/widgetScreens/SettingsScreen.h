#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppConfigInterface.h"
#include "network/NetworkManager.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/display/IpAddressWidget.h"
#include "ui/widgets/display/UptimeWidget.h"
#include "ui/widgets/interactive/BrightnessWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"
#include "utils/ApplicationMetrics.h"

class SettingsScreen : public BaseWidgetScreen {
 public:
    SettingsScreen(LoggerInterface& logger, UiController* uiController, AppConfigInterface& config,
                   NetworkManager& networkManager, ApplicationMetrics& systemMetrics);
    ~SettingsScreen() override = default;

 private:
    void createWidgets() override;
    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
};
