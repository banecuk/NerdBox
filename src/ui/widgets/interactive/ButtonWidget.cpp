#include "ButtonWidget.h"

ButtonWidget::ButtonWidget(DisplayContext& context, const std::string& label,
                           const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           EventType action, ActionCallback callback, uint16_t bgColor,
                           uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      context_(context),
      label_(label),
      action_(action),
      callback_(callback),
      lastTouchTime_(0),
      bgColor_(bgColor),
      textColor_(textColor),
      isPressed_(false),
      pressStartTime_(0) {}

bool ButtonWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!getDimensions().contains(x, y)) {
        return false;
    }

    if (!callback_ || !isInitialized() || !getLcd()) {
        if (getLogger()) {
            getLogger()->debug(
                "ButtonWidget::handleTouch - Rejected (no callback or not initialized)");
        }
        return false;
    }

    // Debouncing
    unsigned long now = millis();
    if (now - lastTouchTime_ < DEBOUNCE_TIME_MS) {
        return false;
    }
    lastTouchTime_ = now;

    // Visual feedback - mark as pressed
    isPressed_ = true;
    pressStartTime_ = now;
    markDirty();  // Force immediate redraw

    // Execute callback
    callback_(action_);

    return true;
}

void ButtonWidget::cleanUp() {
    callback_ = nullptr;
    Widget::cleanUp();
}

void ButtonWidget::setCallback(ActionCallback callback) {
    callback_ = callback;
    markDirty();
}

void ButtonWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    // Auto-release press state after feedback duration
    unsigned long now = millis();
    if (isPressed_ && (now - pressStartTime_ >= PRESS_FEEDBACK_MS)) {
        isPressed_ = false;
        markDirty();  // Trigger redraw to show unpressed state
    }

    uint16_t bgColor = isPressed_ ? TFT_DARKGRAY : bgColor_;
    uint16_t textColor = isPressed_ ? TFT_BLACK : textColor_;

    // Draw button with pressed state
    getLcd()->fillRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, 5,
                            bgColor);

    getLcd()->setTextColor(textColor, bgColor);
    getLcd()->setTextDatum(MC_DATUM);
    getLcd()->setTextSize(1);
    getLcd()->drawString(label_.c_str(), dimensions_.x + dimensions_.width / 2,
                         dimensions_.y + dimensions_.height / 2);
}