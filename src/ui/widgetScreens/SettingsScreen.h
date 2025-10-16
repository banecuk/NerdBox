#pragma once

#include "BaseWidgetScreen.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"
#include "config/AppConfigInterface.h"

class SettingsScreen : public BaseWidgetScreen {
   public:
    SettingsScreen(LoggerInterface& logger, UIController* uiController, AppConfigInterface& config);
    ~SettingsScreen() override = default;

   private:
    void createWidgets() override;
};