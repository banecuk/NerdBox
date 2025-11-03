#include "MetricWidget.h"

#include <algorithm>
#include <cstring>

MetricWidget::MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           uint8_t textSize)
    : Widget(dims, updateIntervalMs), textSize_(textSize) {
    updateDimensions();
    formattedValue_[0] = '\0';  // Initialize buffer
}

void MetricWidget::drawStatic() {
    if (!isInitialized_ || !getLcd()) {
        return;
    }

    LGFX* lcd = getLcd();

    if (dimensionsDirty_) {
        updateDimensions();
    }

    // Clear the entire widget area
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Draw static elements (label if exists)
    if (hasLabel_ && labelWidth_ > 0) {
        // Calculate label position (centered in label area)
        int16_t labelX = dimensions_.x + (labelWidth_ / 2);
        int16_t labelY = dimensions_.y + (dimensions_.height / 2);

        // Draw label text
        lcd->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->setTextSize(1);
        lcd->drawString(label_, labelX, labelY);
    }

    isStaticDrawn_ = true;
    lastDrawnValue_ = -1;
    valueAreaDirty_ = true;
    clearDirty();
}

void MetricWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_) {
        return;
    }

    bool valueChanged = (value_ != lastDrawnValue_);
    bool needsRedraw = forceRedraw || valueChanged || isDirty() || valueAreaDirty_;

    if (needsRedraw) {
        if (valueChanged && lastDrawnValue_ != -1 && !forceRedraw) {
            // Partial redraw - only update what changed
            drawPartialValue(lastDrawnValue_, lastBgColor_);
        } else {
            // Full redraw
            drawValue();
        }
        lastDrawnValue_ = value_;
        clearDirty();
        valueAreaDirty_ = false;
    }
}

void MetricWidget::drawValue() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    lastBgColor_ = getBackgroundColor();

    // Calculate value area
    int16_t valueAreaX, valueAreaY, valueAreaWidth, valueAreaHeight;
    if (hasLabel_) {
        valueAreaX = valueX_;
        valueAreaY = dimensions_.y + BORDER_MARGIN;
        valueAreaWidth = valueWidth_;
        valueAreaHeight = dimensions_.height - (2 * BORDER_MARGIN);
    } else {
        valueAreaX = dimensions_.x + BORDER_MARGIN;
        valueAreaY = dimensions_.y + BORDER_MARGIN;
        valueAreaWidth = dimensions_.width - (2 * BORDER_MARGIN);
        valueAreaHeight = dimensions_.height - (2 * BORDER_MARGIN);
    }

    // Clear value area
    lcd->fillRect(valueAreaX, valueAreaY, valueAreaWidth, valueAreaHeight, lastBgColor_);

    // Get formatted value
    const char* displayText = formatValue();

    lcd->setTextColor(TFT_WHITE, lastBgColor_);
    lcd->setTextDatum(textAlignment_);
    lcd->setTextSize(textSize_);

    // Calculate text position
    int16_t textX, textY;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        textX = valueAreaX + TEXT_MARGIN;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        textX = valueAreaX + valueAreaWidth - TEXT_MARGIN;
    } else {
        textX = valueAreaX + valueAreaWidth / 2;
    }
    textY = dimensions_.y + dimensions_.height / 2;

    lcd->drawString(displayText, textX, textY);
}

void MetricWidget::drawPartialValue(int oldValue, uint16_t oldBgColor) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    uint16_t newBgColor = getBackgroundColor();
    lastBgColor_ = newBgColor;

    // Only redraw if background color changed or value string changed
    if (oldBgColor != newBgColor) {
        // Background color changed, need full value area redraw
        drawValue();
        return;
    }

    // Same background color, just redraw the text
    const char* displayText = formatValue();

    // Calculate value area for text positioning
    int16_t valueAreaX, valueAreaWidth;
    if (hasLabel_) {
        valueAreaX = valueX_;
        valueAreaWidth = valueWidth_;
    } else {
        valueAreaX = dimensions_.x + BORDER_MARGIN;
        valueAreaWidth = dimensions_.width - (2 * BORDER_MARGIN);
    }

    lcd->setTextColor(TFT_WHITE, newBgColor);
    lcd->setTextDatum(textAlignment_);
    lcd->setTextSize(textSize_);

    // Calculate text position
    int16_t textX, textY;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        textX = valueAreaX + TEXT_MARGIN;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        textX = valueAreaX + valueAreaWidth - TEXT_MARGIN;
    } else {
        textX = valueAreaX + valueAreaWidth / 2;
    }
    textY = dimensions_.y + dimensions_.height / 2;

    // Clear only the text area (approximate - could be improved with text bounds)
    int16_t textHeight = 8 * textSize_;                       // Approximate character height
    int16_t textWidth = strlen(displayText) * 6 * textSize_;  // Approximate width
    lcd->fillRect(textX - textWidth / 2, textY - textHeight / 2, textWidth, textHeight, newBgColor);

    lcd->drawString(displayText, textX, textY);
}

void MetricWidget::updateDimensions() {
    valueX_ =
        hasLabel_ ? dimensions_.x + labelWidth_ + SEPARATOR_WIDTH : dimensions_.x + BORDER_MARGIN;
    valueWidth_ = hasLabel_ ? dimensions_.width - labelWidth_ - SEPARATOR_WIDTH - BORDER_MARGIN
                            : dimensions_.width - (2 * BORDER_MARGIN);
    dimensionsDirty_ = false;
    valueAreaDirty_ = true;
}

const char* MetricWidget::formatValue() const {
    if (formatCacheDirty_) {
        if (valueFormatter_) {
            String result = valueFormatter_(value_);
            safeStringCopy(formattedValue_, result.c_str(), sizeof(formattedValue_));
        } else if (unit_[0] == '\0') {
            snprintf(formattedValue_, sizeof(formattedValue_), "%d", value_);
        } else {
            snprintf(formattedValue_, sizeof(formattedValue_), "%d%s", value_, unit_);
        }
        formatCacheDirty_ = false;
    }
    return formattedValue_;
}

uint16_t MetricWidget::getBackgroundColor() const {
    if (maxValue_ <= minValue_) {
        return TFT_BLACK;
    }

    float normalizedPercent;
    if (value_ <= lowerThreshold_) {
        normalizedPercent = 0.0f;
    } else if (value_ >= upperThreshold_) {
        normalizedPercent = 100.0f;
    } else {
        float range = upperThreshold_ - lowerThreshold_;
        if (range <= 0.0f) {
            normalizedPercent = 0.0f;
        } else {
            normalizedPercent = 100.0f * (value_ - lowerThreshold_) / range;
        }
    }

    uint8_t normalizedValue = static_cast<uint8_t>(normalizedPercent);
    return getContext().getColors().getColorFromPercent(normalizedValue, true);
}

bool MetricWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}

void MetricWidget::safeStringCopy(char* dest, const char* src, size_t destSize) const {
    if (destSize > 0) {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
}

void MetricWidget::setValue(int value) {
    if (value_ != value) {
        value_ = value;
        formatCacheDirty_ = true;
        markDirty();
    }
}

void MetricWidget::setUnit(const char* unit) {
    if (strcmp(unit_, unit) != 0) {
        safeStringCopy(unit_, unit, sizeof(unit_));
        formatCacheDirty_ = true;
        markDirty();
    }
}

void MetricWidget::setRange(int minValue, int maxValue) {
    if (minValue_ != minValue || maxValue_ != maxValue) {
        minValue_ = minValue;
        maxValue_ = maxValue;
        markDirty();
    }
}

void MetricWidget::setColorThresholds(float lowerThreshold, float upperThreshold) {
    if (lowerThreshold > upperThreshold) {
        std::swap(lowerThreshold, upperThreshold);
    }

    if (lowerThreshold_ != lowerThreshold || upperThreshold_ != upperThreshold) {
        lowerThreshold_ = lowerThreshold;
        upperThreshold_ = upperThreshold;
        markDirty();
    }
}

void MetricWidget::setLabel(const char* label) {
    if (strcmp(label_, label) != 0) {
        safeStringCopy(label_, label, sizeof(label_));
        bool newHasLabel = (label_[0] != '\0');
        if (hasLabel_ != newHasLabel) {
            hasLabel_ = newHasLabel;
            dimensionsDirty_ = true;
            valueAreaDirty_ = true;
        }
        markDirty();
    }
}

void MetricWidget::setLabelWidth(uint16_t width) {
    if (labelWidth_ != width) {
        labelWidth_ = width;
        dimensionsDirty_ = true;
        valueAreaDirty_ = true;
        markDirty();
    }
}

void MetricWidget::setTextAlignment(uint8_t alignment) {
    if (textAlignment_ != alignment) {
        textAlignment_ = alignment;
        markDirty();
    }
}

void MetricWidget::setValueFormatter(std::function<String(int)> formatter) {
    valueFormatter_ = formatter;
    formatCacheDirty_ = true;
    markDirty();
}

void MetricWidget::setTextSize(uint8_t size) {
    if (textSize_ != size) {
        textSize_ = size;
        markDirty();
    }
}

void MetricWidget::forceRefresh() {
    lastDrawnValue_ = -1;
    formatCacheDirty_ = true;
    valueAreaDirty_ = true;
    markDirty();
}