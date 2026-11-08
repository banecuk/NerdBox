#include "ButtonWidget.h"

#include "ui/resources/FontRegistry.h"
#include "ui/resources/icons_24.h"

namespace {
// Maps the enum to its backing bitmap — nullptr for NONE/unrendered icons.
const uint16_t* iconBitmap(ButtonIcon icon) {
    switch (icon) {
        case ButtonIcon::SETTINGS:
            return icon_gear_24;
        case ButtonIcon::BACK:
            return icon_chevron_left_24;
        case ButtonIcon::FORWARD:
            return icon_chevron_right_24;
        case ButtonIcon::PROCESSES:
            return icon_processes_24;
        case ButtonIcon::NONE:
            return nullptr;
    }
    return nullptr;
}
}  // namespace

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

ButtonWidget::ButtonWidget(const std::string& label, const WidgetInterface::Dimensions& dims,
                           uint32_t updateIntervalMs, TapAction tap, uint16_t bgColor,
                           uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      label_(label),
      icon_(ButtonIcon::NONE),
      tap_(std::move(tap)),
      bgColor_(bgColor),
      textColor_(textColor) {}

ButtonWidget::ButtonWidget(ButtonIcon icon, const std::string& label,
                           const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           TapAction tap, uint16_t bgColor, uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      label_(label),
      icon_(icon),
      tap_(std::move(tap)),
      bgColor_(bgColor),
      textColor_(textColor) {}

// ---------------------------------------------------------------------------
// drawStatic — background fill + border; called once on init / force repaint
// ---------------------------------------------------------------------------

void ButtonWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    // Fill background — anti-aliased corners (see docs-local/03-visual-ux.md
    // V5). Safe on this write-only panel: the AA corner blend is composited
    // against whatever readRect() returns, which is a hard-coded 0 (black)
    // when cfg.readable is false (Panel_LCD::readRect) — correct as long as
    // the screen behind the button is actually black, true everywhere today
    // (Theme::kGround == TFT_BLACK). Revisit once V6 lands non-black cards.
    lcd->fillSmoothRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                             kBorderRadius, bgColor_);

    // 1 px border — barely visible
    lcd->drawRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBorderRadius, kBorderColor);
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void ButtonWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const unsigned long now = millis();

    // Auto-release press feedback. Runs unconditionally (before the dirty
    // early-return below) so it also fires on the one-shot wake-up that
    // handleTouch() schedules via requestWakeAfter() — without that, this
    // widget's updateIntervalMs is 0, needsUpdate() never returns true, and
    // nothing would ever call onDraw() again after the initial pressed draw
    // to notice the feedback window has elapsed.
    if (isPressed_ && (now - pressStartTime_ >= kPressFeedbackMs)) {
        isPressed_ = false;
        markDirty();
    }

    if (!forceRedraw && !isDirty())
        return;

    const uint16_t bg = isPressed_ ? TFT_DARKGRAY : bgColor_;
    const uint16_t fg = isPressed_ ? TFT_BLACK : textColor_;

    // 1. Clear/Fill the button body background canvas
    getLcd()->fillSmoothRoundRect(dimensions_.x, dimensions_.y, dimensions_.width,
                                  dimensions_.height, kBorderRadius, bg);

    // 2. Render the inner content (Icon, Label, or both)
    drawContent(bg, fg);

    // 3. Draw the border LAST so it layer-composes on top of the image canvas
    getLcd()->drawRoundRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                            kBorderRadius, kBorderColor);
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
        if (const uint16_t* bitmap = iconBitmap(icon_)) {
            const int16_t ix = cx - kIconSize / 2;
            const int16_t iy = cy - kIconSize / 2;
            // icons_24.h was generated against a black canvas — key that
            // exact black out as transparent so the button's own fill (no
            // longer always TFT_BLACK, see Theme::kSurface) shows through
            // instead of a solid black square around the glyph.
            lcd->pushImage(ix, iy, kIconSize, kIconSize, bitmap, static_cast<uint16_t>(0x0000));
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

        const uint8_t iconDiam = kIconSize;  // bitmap bounding box
        const int16_t totalW = iconDiam + kIconPad + labelW;
        const int16_t startX = cx - totalW / 2;
        const int16_t iconCx = startX + iconDiam / 2;
        const int16_t labelX = startX + iconDiam + kIconPad + labelW / 2;

        if (const uint16_t* bitmap = iconBitmap(icon_)) {
            const int16_t ix = iconCx - kIconSize / 2;
            const int16_t iy = cy - kIconSize / 2;
            // icons_24.h was generated against a black canvas — key that
            // exact black out as transparent so the button's own fill (no
            // longer always TFT_BLACK, see Theme::kSurface) shows through
            // instead of a solid black square around the glyph.
            lcd->pushImage(ix, iy, kIconSize, kIconSize, bitmap, static_cast<uint16_t>(0x0000));
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

bool ButtonWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    // WidgetManager has already hit-tested (x, y) against this widget's box.
    if (!tap_.isSet() || !isInitialized() || !getLcd())
        return false;

    const unsigned long now = millis();

    isPressed_ = true;
    pressStartTime_ = now;
    // Schedule a wake-up after the feedback window so onDraw() runs again to
    // auto-release the pressed state, even if nothing else redraws the button.
    requestWakeAfter(kPressFeedbackMs);
    markDirty();

    return tap_.fire();
}

void ButtonWidget::cleanUp() {
    tap_ = TapAction{};
    Widget::cleanUp();
}
