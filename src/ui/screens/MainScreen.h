#pragma once

#include "IScreen.h"
#include "core/events/EventBus.h"
#include "services/PcMetrics.h"
#include "ui/UIController.h"
#include "ui/WidgetManager.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"
#include "utils/Logger.h"

class MainScreen : public IScreen {
   public:
    explicit MainScreen(ILogger &logger, PcMetrics &pcMetrics, UIController *uiController);
    ~MainScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

   private:
    ILogger &logger_;
    LGFX *lcd_;
    PcMetrics &pcMetrics_;
    UIController *uiController_;

    WidgetManager widgetManager_;

    int32_t draw_counter_ = 0;

    void createWidgets();
    void handleAction(EventType action);
};