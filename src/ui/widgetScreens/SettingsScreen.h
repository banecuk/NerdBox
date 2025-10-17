#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppConfigInterface.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"

class SettingsScreen : public BaseWidgetScreen {
   public:
    SettingsScreen(LoggerInterface& logger, UiController* uiController,
                   AppConfigInterface& config);
    ~SettingsScreen() override = default;

   private:
    void createWidgets() override;
};