#pragma once

#include "core/EventBus.h"
#include "display/ScreenManager.h"
#include "display/WidgetManager.h"
#include "display/screens/IScreen.h"
#include "display/widgets/ButtonWidget.h"
#include "display/widgets/ClockWidget.h"
#include "services/PcMetrics.h"
#include "utils/Logger.h"

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

    // Widgets as unique_ptr
    std::unique_ptr<ClockWidget> clockWidget_;
    std::unique_ptr<ButtonWidget> button1_;
    std::unique_ptr<ButtonWidget> button2_;

    void handleAction(ActionType action);
};