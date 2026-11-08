#pragma once

#include "config/AppSettings.h"
#include "core/events/EventBus.h"
#include "ui/core/UiController.h"
#include "ui/screens/base/ScreenInterface.h"
#include "ui/widgets/base/TapAction.h"
#include "ui/widgets/layout/WidgetManager.h"
#include "utils/logging/Logger.h"

class BaseWidgetScreen : public ScreenInterface {
 public:
    BaseWidgetScreen(LoggerInterface& logger, UiController& uiController,
                     const AppSettings& config);
    virtual ~BaseWidgetScreen() override;

    void onEnter() override;
    void onExit() override;
    void draw() override;
    void handleTouch(uint16_t x, uint16_t y) override;

 protected:
    virtual void createWidgets() = 0;
    // The one "publish on the EventBus" callback every navigation tap uses
    // (A14) — previously an identical `[this](EventType a) { handleAction(a); }`
    // lambda at each of 11 call sites.
    static TapAction::Callback publisher();
    // A tap that publishes `event` — e.g. tapTo(EventType::SHOW_WEATHER).
    static TapAction tapTo(EventType event) { return TapAction(event, publisher()); }

    // Shared screen chrome — bottom-left back button and bottom-right clock,
    // same box every screen but MainScreen (shares its band with
    // NetworkWidget/NetworkTrafficWidget) and SettingsScreen (its own
    // colour/height/position) used verbatim. Call at the end of
    // createWidgets(); see docs-local/12-code-architecture.md, C3.
    void addBackButton(EventType target = EventType::SHOW_MAIN);
    void addBottomClock();

    LoggerInterface& logger_;
    const AppSettings& config_;

    UiController& uiController_;
    WidgetManager widgetManager_;
};