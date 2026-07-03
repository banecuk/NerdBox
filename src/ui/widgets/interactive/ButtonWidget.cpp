#include "ButtonWidget.h"

#include "core/resources/FontRegistry.h"

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

ButtonWidget::ButtonWidget(DisplayContext& context, const std::string& label,
                           const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           EventType action, ActionCallback callback, uint16_t bgColor,
                           uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      label_(label),
      icon_(ButtonIcon::NONE),
      action_(action),
      callback_(callback),
      bgColor_(bgColor),
      textColor_(textColor) {}

ButtonWidget::ButtonWidget(DisplayContext& context, ButtonIcon icon, const std::string& label,
                           const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           EventType action, ActionCallback callback, uint16_t bgColor,
                           uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      label_(label),
      icon_(icon),
      action_(action),
      callback_(callback),
      bgColor_(bgColor),
      textColor_(textColor) {}

// ---------------------------------------------------------------------------
// drawStatic — background fill + border; called once on init / force repaint
// ---------------------------------------------------------------------------

void ButtonWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;
    LGFX* lcd = getLcd();

    // Fill background
    lcd->fillRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBorderRadius, bgColor_);

    // 1 px border — barely visible
    lcd->drawRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBorderRadius, kBorderColor);

    isStaticDrawn_ = true;
    clearDirty();
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void ButtonWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const unsigned long now = millis();

    // Auto-release press feedback. Runs unconditionally (before the dirty
    // early-return below) so it also fires on the interval-driven wake-up
    // that handleTouch() schedules via setUpdateInterval() — without that,
    // this widget's updateIntervalMs is 0, needsUpdate() never returns true,
    // and nothing would ever call onDraw() again after the initial pressed
    // draw to notice the feedback window has elapsed.
    if (isPressed_ && (now - pressStartTime_ >= PRESS_FEEDBACK_MS)) {
        isPressed_ = false;
        setUpdateInterval(0);  // stop the periodic wake-up until pressed again
        markDirty();
    }

    if (!forceRedraw && !isDirty())
        return;

    const uint16_t bg = isPressed_ ? TFT_DARKGRAY : bgColor_;
    const uint16_t fg = isPressed_ ? TFT_BLACK : textColor_;

    // 1. Clear/Fill the button body background canvas
    getLcd()->fillRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                            kBorderRadius, bg);

    // 2. Render the inner content (Icon, Label, or both)
    drawContent(bg, fg);

    // 3. Draw the border LAST so it layer-composes on top of the image canvas
    getLcd()->drawRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                            kBorderRadius, kBorderColor);

    clearDirty();
}

// ---------------------------------------------------------------------------
// drawContent — icon and/or label, centred in the button
// ---------------------------------------------------------------------------

void ButtonWidget::drawContent(uint16_t bg, uint16_t fg) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const int16_t cx = dimensions_.x + dimensions_.width / 2;
    const int16_t cy = dimensions_.y + dimensions_.height / 2;

    const bool hasIcon = (icon_ != ButtonIcon::NONE);
    const bool hasLabel = !label_.empty();

    if (hasIcon && !hasLabel) {
        // Icon only — centred
        if (icon_ == ButtonIcon::SETTINGS) {
            const int16_t ix = cx - kGearBitmapSize / 2;
            const int16_t iy = cy - kGearBitmapSize / 2;
            lcd->pushImage(ix, iy, kGearBitmapSize, kGearBitmapSize, icon_gear);
        }

    } else if (!hasIcon && hasLabel) {
        // Label only — centred (original behaviour)
        lcd->setTextColor(fg, bg);
        lcd->setTextDatum(MC_DATUM);
        Fonts::loadLabel(lcd);
        lcd->drawString(label_.c_str(), cx, cy);
        Fonts::unload(lcd);

    } else if (hasIcon && hasLabel) {
        // Icon left + gap + label right, combined block centred in button
        Fonts::loadLabel(lcd);
        const int16_t labelW = static_cast<int16_t>(lcd->textWidth(label_.c_str()));
        Fonts::unload(lcd);

        const uint8_t iconDiam = dimensions_.height - 12;  // diameter of icon bounding box
        const int16_t totalW = iconDiam + kIconPad + labelW;
        const int16_t startX = cx - totalW / 2;
        const int16_t iconCx = startX + iconDiam / 2;
        const int16_t labelX = startX + iconDiam + kIconPad + labelW / 2;

        if (icon_ == ButtonIcon::SETTINGS) {
            const int16_t ix = iconCx - kGearBitmapSize / 2;
            const int16_t iy = cy - kGearBitmapSize / 2;
            lcd->pushImage(ix, iy, kGearBitmapSize, kGearBitmapSize, icon_gear);
        }

        lcd->setTextColor(fg, bg);
        lcd->setTextDatum(MC_DATUM);
        Fonts::loadLabel(lcd);
        lcd->drawString(label_.c_str(), labelX, cy);
        Fonts::unload(lcd);
    }
}

// ---------------------------------------------------------------------------
// Touch handling
// ---------------------------------------------------------------------------

bool ButtonWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!getDimensions().contains(x, y))
        return false;

    if (!callback_ || !isInitialized() || !getLcd())
        return false;

    const unsigned long now = millis();

    isPressed_ = true;
    pressStartTime_ = now;
    // Schedule a wake-up after the feedback window so onDraw() runs again to
    // auto-release the pressed state, even if nothing else redraws the button.
    setUpdateInterval(PRESS_FEEDBACK_MS);
    markDirty();

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
