#include "SwitchWidget.h"

#include "core/resources/FontRegistry.h"
#include "ui/widgets/base/WidgetPainter.h"

SwitchWidget::SwitchWidget(const WidgetInterface::Dimensions& dims, std::string label,
                           GetStateFn getState, SetStateFn setState)
    : Widget(dims, 0),
      label_(std::move(label)),
      getState_(std::move(getState)),
      setState_(std::move(setState)) {}

void SwitchWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    WidgetPainter::drawCaptionLabel(lcd, dimensions_.x, dimensions_.y, label_.c_str());

    hasDrawnOnce_ = false;  // force full track redraw on next onDraw()
}

void SwitchWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool current = getState_ ? getState_() : false;
    if (!forceRedraw && hasDrawnOnce_ && current == lastDrawnState_) {
        clearDirty();
        return;
    }

    drawTrack(current);

    lastDrawnState_ = current;
    hasDrawnOnce_ = true;
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void SwitchWidget::drawTrack(bool on) {
    const uint16_t trackY = dimensions_.y + kLabelH + kGap;
    const uint16_t trackH = dimensions_.height - kLabelH - kGap;

    const uint16_t bgColor = on ? kOnColor : kOffColor;
    const uint16_t textColor = on ? TFT_BLACK : Colors::kInactiveText;

    WidgetPainter::drawPillToggle(getLcd(), dimensions_.x, trackY, dimensions_.width, trackH,
                                  kRadius, bgColor, textColor, on ? "ON" : "OFF");
}

bool SwitchWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!isInitialized_ || !getLcd() || !getState_ || !setState_)
        return false;

    const uint16_t trackY = dimensions_.y + kLabelH + kGap;
    const uint16_t trackH = dimensions_.height - kLabelH - kGap;

    if (x >= dimensions_.x && x < static_cast<uint16_t>(dimensions_.x + dimensions_.width) &&
        y >= trackY && y < static_cast<uint16_t>(trackY + trackH)) {
        setState_(!getState_());
        markDirty();
        return true;
    }

    return false;
}
