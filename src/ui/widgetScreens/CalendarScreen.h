#pragma once

#include "BaseWidgetScreen.h"
#include "config/AppSettings.h"
#include "ui/widgets/display/CalendarWidget.h"
#include "ui/widgets/display/ClockWidget.h"
#include "ui/widgets/interactive/ButtonWidget.h"

// Month-grid calendar screen. Entered by tapping the clock on the main
// screen. Prev/next-month arrow buttons flank the month title; back button
// and clock share the bottom band with every other widget screen.
class CalendarScreen : public BaseWidgetScreen {
 public:
    CalendarScreen(LoggerInterface& logger, UiController* uiController, const AppSettings& config);
    ~CalendarScreen() override = default;

 private:
    void createWidgets() override;
};
