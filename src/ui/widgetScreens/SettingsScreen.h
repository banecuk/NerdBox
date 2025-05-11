#pragma once

#include "BaseWidgetScreen.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"

class SettingsScreen : public BaseWidgetScreen {
   public:
    SettingsScreen(ILogger& logger, UIController* uiController);
    ~SettingsScreen() override = default;

   private:
    void createWidgets() override;
};