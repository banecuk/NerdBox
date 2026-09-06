#pragma once

#include <array>

#include "core/ScreenRegistry.h"
#include "core/events/EventBus.h"
#include "utils/logging/Logger.h"

// Forward declarations
class UiController;
class DisplayManager;

// UiEventHandler — translates EventBus events into UiController / DisplayManager actions.
//
// The SHOW_* → screen subscriptions are generated from core/ScreenRegistry.h's
// kScreens table (one requestScreen(descriptor.screen) handler each, skipping
// entries with no event such as BOOT), plus three non-screen subscriptions
// registered by hand: NONE (log only, debug aid), RESET_DEVICE → resetDevice(),
// CYCLE_BRIGHTNESS → cycleBrightness().
class UiEventHandler {
 public:
    UiEventHandler(UiController* uiController, LoggerInterface& logger);
    ~UiEventHandler();

    void registerHandlers();
    void resetDevice();
    void cycleBrightness();

 private:
    UiController* uiController_;
    LoggerInterface& logger_;

    struct Subscription {
        EventType type;
        EventBus::SubscriptionId id;
    };
    // kScreenCount includes BOOT, which has no event (skipped below); the other
    // kScreenCount - 1 screens each get a subscription, plus 3 registered by hand.
    static constexpr size_t kSubscriptionCount = kScreenCount - 1 + 3;
    std::array<Subscription, kSubscriptionCount> subscriptions_{};
};
