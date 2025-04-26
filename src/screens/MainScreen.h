#pragma once

#include "core/display/ScreenManager.h"
#include "core/display/WidgetManager.h"
#include "core/utils/Logger.h"
#include "screens/IScreen.h"
#include "screens/widgets/ButtonWidget.h"
#include "screens/widgets/ClockWidget.h"
#include "services/PcMetrics.h"

class MainScreen : public IScreen {
   public:
    explicit MainScreen(ILogger &logger, LGFX *lcd, PcMetrics &hmData,
                        ScreenManager *screenManager);
    ~MainScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

   private:
    ILogger &logger_;
    LGFX *lcd_;
    PcMetrics &pcMetrics_;
    ScreenManager *screenManager_;

    WidgetManager widgetManager_;

    // --- Widgets ---
    ClockWidget clockWidget_;
    ButtonWidget button1_;
    ButtonWidget button2_;

    void handleAction(ActionType action);
};