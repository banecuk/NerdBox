#pragma once

#include "core/events/EventBus.h"
#include "ui/UIController.h"
#include "ui/WidgetManager.h"
#include "ui/screens/IScreen.h"
#include "utils/Logger.h"

class BaseWidgetScreen : public IScreen {
   public:
    BaseWidgetScreen(ILogger& logger, UIController* uiController);
    virtual ~BaseWidgetScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

   protected:
    virtual void
    createWidgets() = 0;  // Pure virtual to force derived classes to implement
    void handleAction(EventType action);

    ILogger& logger_;
    LGFX* lcd_;
    UIController* uiController_;
    WidgetManager widgetManager_;
};