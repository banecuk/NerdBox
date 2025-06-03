#include "SingleValueWidget.h"

SingleValueWidget::SingleValueWidget(const Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs) {
    updateDimensions();
}

void SingleValueWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;
    
    if (dimensionsDirty_) {
        updateDimensions();
    }
    
    // Draw static elements (border and label if exists)
    if (hasLabel_ && labelWidth_ > 0) {
        // Draw label area (left part)
        lcd_->fillRect(dimensions_.x, dimensions_.y, 
                      labelWidth_, dimensions_.height, 
                      TFT_BLACK);
        
        // Draw label text
        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextDatum(CL_DATUM);
        lcd_->setTextSize(1);
        
        int16_t labelX = dimensions_.x + (labelWidth_ / 2);
        int16_t labelY = dimensions_.y + (dimensions_.height / 2);
        lcd_->drawString(label_, labelX, labelY);
        
        // Draw separator line
        lcd_->drawFastVLine(dimensions_.x + labelWidth_, dimensions_.y, dimensions_.height, TFT_DARKGREY);
    }
    
    staticDrawn_ = true;
}

void SingleValueWidget::draw(bool forceRedraw) {
    if (!initialized_ || !lcd_) return;
    
    if (forceRedraw || needsUpdate()) {
        if (dimensionsDirty_) {
            updateDimensions();
        }
        if (textSizeDirty_) {
            updateTextSize();
        }
        drawValue();
        lastUpdateTimeMs_ = millis();
    }
}

void SingleValueWidget::drawValue() {
    uint16_t bgColor = getBackgroundColor();
    uint16_t textColor = getTextColor(bgColor);
    
    // Fill background
    lcd_->fillRect(valueX_, dimensions_.y + 1,
                  valueWidth_, dimensions_.height - 2,
                  bgColor);
    
    // Prepare value text
    String valueText = String(value_) + unit_;
    
    // Set text properties
    lcd_->setTextColor(textColor, bgColor);
    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextSize(optimalTextSize_);
    
    // Draw the text centered in the value area
    int16_t centerX = valueX_ + valueWidth_ / 2;
    int16_t centerY = dimensions_.y + dimensions_.height / 2;
    lcd_->drawString(valueText, centerX, centerY);
}

void SingleValueWidget::updateDimensions() {
    valueX_ = hasLabel_ ? dimensions_.x + labelWidth_ + 1 : dimensions_.x + 1;
    valueWidth_ = hasLabel_ ? dimensions_.width - labelWidth_ - 2 : dimensions_.width - 2;
    dimensionsDirty_ = false;
    textSizeDirty_ = true; // Dimensions changed, so text size might need update
}

void SingleValueWidget::updateTextSize() {
    if (!lcd_) return;
    
    String valueText = String(value_) + unit_;
    optimalTextSize_ = 1;
    lcd_->setTextSize(optimalTextSize_);
    int16_t textWidth = lcd_->textWidth(valueText);
    
    // Increase text size if it fits
    while (optimalTextSize_ < 4 && 
           textWidth < (valueWidth_ - 10) && 
           lcd_->fontHeight() < (dimensions_.height - 10)) {
        optimalTextSize_++;
        lcd_->setTextSize(optimalTextSize_);
        textWidth = lcd_->textWidth(valueText);
    }
    
    textSizeDirty_ = false;
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
        textSizeDirty_ = true; // Value changed, text size might need update
        draw();
    }
}

void SingleValueWidget::setUnit(const String& unit) {
    if (unit_ != unit) {
        unit_ = unit;
        textSizeDirty_ = true; // Unit changed, text size might need update
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

void SingleValueWidget::setLabel(const String& label) {
    if (label_ != label) {
        label_ = label;
        hasLabel_ = !label_.isEmpty();
        dimensionsDirty_ = true;
        if (staticDrawn_) {
            drawStatic(); // Redraw static elements if label changed
            draw(true);  // Force redraw of value
        }
    }
}

void SingleValueWidget::setLabelWidth(uint16_t width) {
    if (labelWidth_ != width) {
        labelWidth_ = width;
        dimensionsDirty_ = true;
        if (staticDrawn_) {
            drawStatic(); // Redraw static elements if label width changed
            draw(true);  // Force redraw of value
        }
    }
}