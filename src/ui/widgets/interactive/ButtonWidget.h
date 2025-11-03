#pragma once

#include <functional>
#include <string>

#include "core/events/EventTypes.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"

class ButtonWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    ButtonWidget(DisplayContext& context, const std::string& label,
                 const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs = 0,
                 EventType action = EventType::NONE, ActionCallback callback = nullptr,
                 uint16_t bgColor = TFT_DARKGRAY, uint16_t textColor = TFT_WHITE);

    void cleanUp() override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    void setCallback(ActionCallback callback);

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    DisplayContext& context_;
    std::string label_;
    unsigned long lastTouchTime_;
    EventType action_;
    ActionCallback callback_;
    uint16_t bgColor_;
    uint16_t textColor_;

    bool isPressed_ = false;
    unsigned long pressStartTime_ = 0;
    static constexpr unsigned long DEBOUNCE_TIME_MS = 200;
    static constexpr unsigned long PRESS_FEEDBACK_MS = 100;
};