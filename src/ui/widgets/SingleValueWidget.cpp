#include "SingleValueWidget.h"

SingleValueWidget::SingleValueWidget(const Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs) {}

void SingleValueWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;
    
    // Draw static elements (border)
    // lcd_->drawRect(dimensions_.x, dimensions_.y, 
    //               dimensions_.width, dimensions_.height, 
    //               TFT_DARKGREY);
    
    staticDrawn_ = true;
}

void SingleValueWidget::draw(bool forceRedraw) {
    if (!initialized_ || !lcd_) return;
    
    if (forceRedraw || needsUpdate()) {
        drawValue();
        lastUpdateTimeMs_ = millis();
    }
}

void SingleValueWidget::drawValue() {
    uint16_t bgColor = getBackgroundColor();
    uint16_t textColor = getTextColor(bgColor);
    
    // Fill background
    lcd_->fillRect(dimensions_.x + 1, dimensions_.y + 1,
                  dimensions_.width - 2, dimensions_.height - 2,
                  bgColor);
    
    // Prepare value text
    String valueText = String(value_) + unit_;
    
    // Set text properties
    lcd_->setTextColor(textColor, bgColor);
    lcd_->setTextDatum(MC_DATUM);
    
    // Calculate optimal text size
    uint8_t textSize = 1;
    lcd_->setTextSize(textSize);
    int16_t textWidth = lcd_->textWidth(valueText);
    
    // Increase text size if it fits
    while (textSize < 4 && 
           textWidth < (dimensions_.width - 10) && 
           lcd_->fontHeight() < (dimensions_.height - 10)) {
        textSize++;
        lcd_->setTextSize(textSize);
        textWidth = lcd_->textWidth(valueText);
    }
    
    // Draw the text centered
    int16_t centerX = dimensions_.x + dimensions_.width / 2;
    int16_t centerY = dimensions_.y + dimensions_.height / 2;
    lcd_->drawString(valueText, centerX, centerY);
}

uint16_t SingleValueWidget::getBackgroundColor() const {
    if (maxValue_ <= minValue_) return TFT_BLACK;
    
    float normalizedValue = static_cast<float>(value_ - minValue_) / (maxValue_ - minValue_);
    
    if (normalizedValue <= greenThreshold_) {
        return TFT_DARKGREEN;
    } else if (normalizedValue <= yellowThreshold_) {
        return TFT_GREEN;
    } else if (normalizedValue <= redThreshold_) {
        return TFT_YELLOW;
    } else {
        return TFT_RED;
    }
}

uint16_t SingleValueWidget::getTextColor(uint16_t bgColor) const {
    // Return contrasting text color based on background
    switch (bgColor) {
        case TFT_DARKGREEN:
        case TFT_RED:
            return TFT_WHITE;
        case TFT_GREEN:
        case TFT_YELLOW:
            return TFT_BLACK;
        default:
            return TFT_WHITE;
    }
}

bool SingleValueWidget::handleTouch(uint16_t x, uint16_t y) {
    return false; // No touch interaction
}

void SingleValueWidget::setValue(int value) {
    if (value_ != value) {
        value_ = value;
        draw();
    }
}

void SingleValueWidget::setUnit(const String& unit) {
    if (unit_ != unit) {
        unit_ = unit;
        draw();
    }
}

void SingleValueWidget::setRange(int minValue, int maxValue) {
    if (minValue_ != minValue || maxValue_ != maxValue) {
        minValue_ = minValue;
        maxValue_ = maxValue;
        draw();
    }
}

void SingleValueWidget::setColorThresholds(float greenThreshold, float yellowThreshold, float redThreshold) {
    greenThreshold_ = greenThreshold;
    yellowThreshold_ = yellowThreshold;
    redThreshold_ = redThreshold;
    draw();
}