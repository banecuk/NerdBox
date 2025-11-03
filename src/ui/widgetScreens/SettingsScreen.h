#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppConfigInterface.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

class SettingsScreen : public BaseWidgetScreen {
 public:
    SettingsScreen(LoggerInterface& logger, UiController* uiController, AppConfigInterface& config);
    ~SettingsScreen() override = default;

 private:
    void createWidgets() override;
};