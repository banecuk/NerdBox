#include "ButtonWidget.h"

ButtonWidget::ButtonWidget(const std::string& label, const Dimensions& dims,
                           uint32_t updateIntervalMs, EventType action,
                           ActionCallback callback,
                           uint16_t bgColor, uint16_t textColor)
    : Widget(dims, updateIntervalMs),
      label_(label),
      action_(action),
      callback_(callback),
      lastTouchTime_(0),
      bgColor_(bgColor),
      textColor_(textColor) {}

void ButtonWidget::initialize(LGFX* lcd, LoggerInterface& logger) {
    Widget::initialize(lcd, logger);
}

void ButtonWidget::draw(bool forceRedraw /* = false */) {
    if (!initialized_ || !callback_) return;

    if (bgColor_ == TFT_BLACK) {
        // Draw outlined button
        lcd_->drawRoundRect(dimensions_.x, dimensions_.y, dimensions_.width,
                           dimensions_.height, 5, TFT_DARKGRAY);
    } else {
        // Draw filled button
        lcd_->fillRoundRect(dimensions_.x, dimensions_.y, dimensions_.width,
                           dimensions_.height, 5, bgColor_);
    }

    lcd_->setTextColor(textColor_, bgColor_ == TFT_BLACK ? TFT_BLACK : bgColor_);
    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextSize(1);
    uint16_t textX = dimensions_.x + dimensions_.width / 2;
    uint16_t textY = dimensions_.y + dimensions_.height / 2;
    lcd_->drawString(label_.c_str(), textX, textY);

    lastUpdateTimeMs_ = millis();
}

void ButtonWidget::setCallback(ActionCallback callback) { callback_ = callback; }


bool ButtonWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!callback_) {
        logger_->debug("Button callback empty (normal during transitions)");
        return false;
    }

    if (!initialized_ || !lcd_ || !callback_) { 
        logger_->debug("ButtonWidget::handleTouch - Rejected");
        return false;
    }

    constexpr unsigned long debounceTime = 200; // ms
    unsigned long now = millis();
    
    if (now - lastTouchTime_ < debounceTime) return false;
    
    lastTouchTime_ = now;
    
    if (callback_) {
        callback_(action_);
        return true;
    }
    return false;
}

void ButtonWidget::cleanUp() { 
    callback_ = nullptr;
    Widget::cleanUp();
}
