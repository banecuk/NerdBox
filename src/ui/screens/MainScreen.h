#pragma once

#include "IScreen.h"
#include "core/EventBus.h"
#include "services/PcMetrics.h"
#include "ui/UIController.h"
#include "ui/WidgetManager.h"
#include "ui/widgets/ButtonWidget.h"
#include "ui/widgets/ClockWidget.h"
#include "utils/Logger.h"

class MainScreen : public IScreen {
   public:
    explicit MainScreen(ILogger &logger, LGFX *lcd, PcMetrics &hmData,
                        UIController *uiController);
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

    // Widgets as unique_ptr
    std::unique_ptr<ClockWidget> clockWidget_;
    std::unique_ptr<ButtonWidget> button1_;
    std::unique_ptr<ButtonWidget> button2_;

    void handleAction(ActionType action);
};