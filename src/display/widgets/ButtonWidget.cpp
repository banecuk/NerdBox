#include "ButtonWidget.h"

ButtonWidget::ButtonWidget(const std::string& label, const Dimensions& dims,
                           uint32_t updateIntervalMs, ActionType action,
                           ActionCallback callback)
    : BaseWidget(dims, updateIntervalMs),
      label_(label),
      action_(action),
      callback_(callback),
      lastTouchTime_(0) {}

void ButtonWidget::initialize(LGFX* lcd, ILogger& logger) {
    BaseWidget::initialize(lcd, logger);
}

void ButtonWidget::draw(bool forceRedraw /* = false */) {
    if (!initialized_ || !lcd_) return;

    uint16_t bgColor = TFT_DARKGRAY;
    uint16_t textColor = TFT_WHITE;

    lcd_->fillRoundRect(dimensions_.x, dimensions_.y, dimensions_.width,
                        dimensions_.height, 5, bgColor);

    lcd_->setTextColor(textColor, bgColor);
    lcd_->setTextDatum(MC_DATUM);
    uint16_t textX = dimensions_.x + dimensions_.width / 2;
    uint16_t textY = dimensions_.y + dimensions_.height / 2;
    lcd_->drawString(label_.c_str(), textX, textY);

    lastUpdateTimeMs_ = millis();
}

void ButtonWidget::setCallback(ActionCallback callback) { callback_ = callback; }

bool ButtonWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!initialized_) return false;

    static unsigned long lastTouch = 0;
    if (millis() - lastTouch < 200) return false;
    lastTouch = millis();

    if (callback_) {
        callback_(action_);
        return true;
    }
    return false;
}

void ButtonWidget::cleanUp() { BaseWidget::cleanUp(); }
