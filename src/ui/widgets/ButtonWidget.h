#pragma once

#include <string>

#include "Widget.h"
#include "core/events/EventTypes.h"
#include "ui/DisplayContext.h"

class ButtonWidget : public Widget {
   public:
    typedef std::function<void()> Callback;

    using ActionCallback = std::function<void(EventType)>;

    ButtonWidget(DisplayContext& context, const std::string& label, const Dimensions& dims,
                 uint32_t updateIntervalMs = 0, EventType action = EventType::NONE,
                 ActionCallback callback = nullptr, uint16_t bgColor = TFT_DARKGRAY,
                 uint16_t textColor = TFT_WHITE);

    // void initialize(DisplayContext& context) override;

    void draw(bool forceRedraw = false) override;
    void cleanUp() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

    void setCallback(ActionCallback callback);

   private:
    DisplayContext& context_;
    std::string label_;
    unsigned long lastTouchTime_;  // Track last touch time for debouncing

    EventType action_;
    ActionCallback callback_;
    uint16_t bgColor_;
    uint16_t textColor_;
};