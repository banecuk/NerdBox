#pragma once

#include "config/AppConfigInterface.h"
#include "core/events/EventBus.h"
#include "ui/UiController.h"
#include "ui/WidgetManager.h"
#include "ui/screens/ScreenInterface.h"
#include "utils/Logger.h"

class BaseWidgetScreen : public ScreenInterface {
   public:
    BaseWidgetScreen(LoggerInterface& logger, UiController* uiController,
                     AppConfigInterface& config);
    virtual ~BaseWidgetScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

   protected:
    virtual void
    createWidgets() = 0;  // Pure virtual to force derived classes to implement
    void handleAction(EventType action);

    LoggerInterface& logger_;
    AppConfigInterface& config_;

    LGFX* lcd_;
    UiController* uiController_;
    WidgetManager widgetManager_;
};