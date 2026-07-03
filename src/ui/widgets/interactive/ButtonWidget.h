#pragma once

#include <functional>
#include <string>

#include "core/events/EventTypes.h"
#include "core/resources/gear_icon_40.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"

// Supported icon glyphs.
enum class ButtonIcon : uint8_t {
    NONE,
    SETTINGS,  // Gear / cog
};

class ButtonWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    // label only (original constructor — backward compatible)
    ButtonWidget(DisplayContext& context, const std::string& label,
                 const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs = 0,
                 EventType action = EventType::NONE, ActionCallback callback = nullptr,
                 uint16_t bgColor = TFT_DARKGRAY, uint16_t textColor = TFT_WHITE);

    // icon + optional label
    ButtonWidget(DisplayContext& context, ButtonIcon icon, const std::string& label,
                 const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs = 0,
                 EventType action = EventType::NONE, ActionCallback callback = nullptr,
                 uint16_t bgColor = TFT_DARKGRAY, uint16_t textColor = TFT_WHITE);

    void cleanUp() override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    void setCallback(ActionCallback callback);

 protected:
    void drawStatic() override;
    void onDraw(bool forceRedraw) override;

 private:
    void drawContent(uint16_t bg, uint16_t fg);

    static constexpr uint8_t kGearBitmapSize = 40;  // gear_icon_40.h pixel dimensions

    std::string label_;
    ButtonIcon icon_;
    unsigned long lastTouchTime_;
    EventType action_;
    ActionCallback callback_;
    uint16_t bgColor_;
    uint16_t textColor_;

    bool isPressed_ = false;
    unsigned long pressStartTime_ = 0;

    // Border colour — very dark grey, barely visible against black background
    static constexpr uint16_t kBorderColor = 0x2965;  // ~RGB(40,44,40)
    static constexpr uint16_t kBorderRadius = 5;
    static constexpr uint8_t kIconPad = 4;  // px between icon and label

    static constexpr unsigned long DEBOUNCE_TIME_MS = 200;
    static constexpr unsigned long PRESS_FEEDBACK_MS = 100;
};
