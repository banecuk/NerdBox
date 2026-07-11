#pragma once

#include "config/AppSettings.h"
#include "core/events/EventBus.h"
#include "ui/core/UiController.h"
#include "ui/screens/ScreenInterface.h"
#include "ui/widgets/layout/WidgetManager.h"
#include "utils/Logger.h"

class BaseWidgetScreen : public ScreenInterface {
 public:
    BaseWidgetScreen(LoggerInterface& logger, UiController* uiController,
                     const AppSettings& config);
    virtual ~BaseWidgetScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

 protected:
    virtual void createWidgets() = 0;
    void handleAction(EventType action);

    LoggerInterface& logger_;
    const AppSettings& config_;

    UiController* uiController_;
    WidgetManager widgetManager_;
};