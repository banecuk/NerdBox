#pragma once

#include <functional>
#include <utility>

#include "core/events/EventTypes.h"

// What a tappable widget does when tapped (A14): an EventType plus the
// callback that receives it. Replaces the `using ActionCallback = ...` alias
// and `action_`/`callback_` member pair that 13 widgets each declared, with
// identical `if (!callback_) return false; callback_(action_); return true;`
// handleTouch() bodies — those are now `return tap_.fire();`.
//
// Screens build navigation taps with BaseWidgetScreen::tapTo(EventType)
// (publishes on the EventBus); a screen-local action that isn't an EventBus
// round-trip (calendar month arrows, WiFi rescan) uses TapAction::local().
// Also the hook for V8's tap feedback (03-visual-ux.md): one place every tap
// already goes through.
class TapAction {
 public:
    using Callback = std::function<void(EventType)>;

    TapAction() = default;
    TapAction(EventType action, Callback callback)
        : action_(action), callback_(std::move(callback)) {}

    // A tap that runs `fn` directly, with no EventType involved.
    static TapAction local(std::function<void()> fn) {
        return TapAction(EventType::NONE, [fn = std::move(fn)](EventType) { fn(); });
    }

    // True when there's something to do — widgets without one stay
    // untappable (handleTouch() returns false so the touch falls through).
    bool isSet() const { return static_cast<bool>(callback_); }

    // Runs the action; returns whether it did, ready to be handleTouch()'s
    // return value.
    bool fire() const {
        if (!callback_) {
            return false;
        }
        callback_(action_);
        return true;
    }

    EventType action() const { return action_; }

 private:
    EventType action_ = EventType::NONE;
    Callback callback_;
};
