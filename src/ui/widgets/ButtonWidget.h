#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include <string>

#include "Widget.h"
#include "core/events/EventTypes.h"

class ButtonWidget : public Widget {
   public:
    typedef std::function<void()> Callback;

    using ActionCallback = std::function<void(EventType)>;

    ButtonWidget(const std::string& label, const Dimensions& dims,
                 uint32_t updateIntervalMs = 0,
                 EventType action = EventType::NONE,
                 ActionCallback callback = nullptr);

    void initialize(LGFX* lcd, ILogger& logger) override;

    void draw(bool forceRedraw = false) override;
    void cleanUp() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

    void setCallback(ActionCallback callback);

   private:
    std::string label_;
    unsigned long lastTouchTime_;  // Track last touch time for debouncing

    EventType action_;
    ActionCallback callback_;
};

#endif  // BUTTONWIDGET_H