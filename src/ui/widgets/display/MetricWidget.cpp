#include "MetricWidget.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

MetricWidget::MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           uint8_t textSize)
    : Widget(dims, updateIntervalMs), textSize_(textSize), useDimColors_(false) {
    updateDimensionCache();
    formattedValue_[0] = '\0';
}

void MetricWidget::drawStatic() {
    if (!isInitialized_ || !getLcd()) {
        return;
    }

    LGFX* lcd = getLcd();

    if (dimensionsDirty_) {
        updateDimensionCache();
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
    bool baseWidgetDirty = isDirty();

    // Always draw on first render (lastDrawnValue_ == -1)
    bool firstRender = (lastDrawnValue_ == -1);

    // Only check other dirty flags if value changed or forced
    if (!firstRender && !valueChanged && !forceRedraw && !baseWidgetDirty && !valueAreaDirty_) {
        return;  // Nothing to do
    }

    if (valueChanged && lastDrawnValue_ != -1 && !forceRedraw && !valueAreaDirty_) {
        // Optimization: only redraw text if just the value changed
        renderValueTextOnly();
    } else {
        // Full value area redraw (background + text)
        renderValueArea();
    }
    lastDrawnValue_ = value_;
    clearDirty();
    valueAreaDirty_ = false;
}

void MetricWidget::renderValueArea() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    lastBgColor_ = calculateBackgroundColor();

    // Calculate value area bounds
    int16_t areaX, areaY, areaWidth, areaHeight;
    if (hasLabel_) {
        areaX = valueX_;
        areaY = dimensions_.y + BORDER_MARGIN;
        areaWidth = valueWidth_;
        areaHeight = dimensions_.height - (2 * BORDER_MARGIN);
    } else {
        areaX = dimensions_.x + BORDER_MARGIN;
        areaY = dimensions_.y + BORDER_MARGIN;
        areaWidth = dimensions_.width - (2 * BORDER_MARGIN);
        areaHeight = dimensions_.height - (2 * BORDER_MARGIN);
    }

    // Clear value area with background color
    lcd->fillRect(areaX, areaY, areaWidth, areaHeight, lastBgColor_);

    // Get formatted value text
    const char* displayText = getFormattedValueText();

    if (displayText == nullptr || strlen(displayText) == 0) {
        // Serial.printf("WARNING: MetricWidget displayText is empty for value: %d\n", value_);
        displayText = "0";  // Fallback to show something
    }

    lcd->setTextColor(TFT_WHITE, lastBgColor_);
    lcd->setTextDatum(textAlignment_);
    lcd->setTextSize(textSize_);

    // Calculate text position based on alignment
    int16_t textX, textY;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        textX = areaX + TEXT_MARGIN;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        textX = areaX + areaWidth - TEXT_MARGIN;
    } else {
        textX = areaX + areaWidth / 2;
    }
    textY = dimensions_.y + dimensions_.height / 2;

    lcd->drawString(displayText, textX, textY);
}

void MetricWidget::renderValueTextOnly() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    uint16_t newBgColor = calculateBackgroundColor();

    // If background color changed, need full redraw
    if (lastBgColor_ != newBgColor) {
        lastBgColor_ = newBgColor;
        renderValueArea();
        return;
    }

    // Background color unchanged, only redraw text
    const char* displayText = getFormattedValueText();

    // Calculate value area for text positioning
    int16_t areaX, areaWidth;
    if (hasLabel_) {
        areaX = valueX_;
        areaWidth = valueWidth_;
    } else {
        areaX = dimensions_.x + BORDER_MARGIN;
        areaWidth = dimensions_.width - (2 * BORDER_MARGIN);
    }

    lcd->setTextColor(TFT_WHITE, newBgColor);
    lcd->setTextDatum(textAlignment_);
    lcd->setTextSize(textSize_);

    // Calculate text position based on alignment
    int16_t textX, textY;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        textX = areaX + TEXT_MARGIN;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        textX = areaX + areaWidth - TEXT_MARGIN;
    } else {
        textX = areaX + areaWidth / 2;
    }
    textY = dimensions_.y + dimensions_.height / 2;

    // Estimate text dimensions for clearing
    int16_t textHeight = 8 * textSize_;
    int16_t textWidth = strlen(displayText) * 6 * textSize_;

    // Clear text area based on alignment
    int16_t clearX, clearY;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        clearX = textX;
        clearY = textY - textHeight / 2;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        clearX = textX - textWidth;
        clearY = textY - textHeight / 2;
    } else {
        clearX = textX - textWidth / 2;
        clearY = textY - textHeight / 2;
    }

    lcd->fillRect(clearX, clearY, textWidth, textHeight, newBgColor);

    lcd->drawString(displayText, textX, textY);
}

void MetricWidget::updateDimensionCache() {
    valueX_ =
        hasLabel_ ? dimensions_.x + labelWidth_ + SEPARATOR_WIDTH : dimensions_.x + BORDER_MARGIN;
    valueWidth_ = hasLabel_ ? dimensions_.width - labelWidth_ - SEPARATOR_WIDTH - BORDER_MARGIN
                            : dimensions_.width - (2 * BORDER_MARGIN);
    dimensionsDirty_ = false;
    valueAreaDirty_ = true;
}

const char* MetricWidget::getFormattedValueText() const {
    if (formatCacheDirty_) {
        switch (valueFormat_) {
            case ValueFormat::kPercent:
                snprintf(formattedValue_, sizeof(formattedValue_), "%d%%", value_);
                break;
            case ValueFormat::kRpm:
                snprintf(formattedValue_, sizeof(formattedValue_), "%d RPM", value_);
                break;
            case ValueFormat::kWatts:
                snprintf(formattedValue_, sizeof(formattedValue_), "%dW", value_);
                break;
            case ValueFormat::kCelsius:
                snprintf(formattedValue_, sizeof(formattedValue_), "%d\xC2\xB0""C", value_);
                break;
            case ValueFormat::kMB:
                snprintf(formattedValue_, sizeof(formattedValue_), "%d MB", value_);
                break;
            case ValueFormat::kDefault:
            default:
                if (unit_[0] == '\0') {
                    snprintf(formattedValue_, sizeof(formattedValue_), "%d", value_);
                } else {
                    snprintf(formattedValue_, sizeof(formattedValue_), "%d%s", value_, unit_);
                }
                break;
        }
        formatCacheDirty_ = false;
    }
    return formattedValue_;
}

uint16_t MetricWidget::calculateBackgroundColor() const {
    if (maxValue_ <= minValue_) {
        return TFT_BLACK;
    }

    float normalizedPercent;

    if (reverseThresholds_) {
        // REVERSE LOGIC: Warning color for LOW values
        if (value_ >= upperThreshold_) {
            normalizedPercent = 0.0f;  // Good (green) when value is HIGH
        } else if (value_ <= lowerThreshold_) {
            normalizedPercent = 100.0f;  // Bad (red) when value is LOW
        } else {
            float range = upperThreshold_ - lowerThreshold_;
            if (range <= 0.0f) {
                normalizedPercent = 0.0f;
            } else {
                // Invert the calculation for reverse thresholds
                normalizedPercent = 100.0f * (upperThreshold_ - value_) / range;
            }
        }
    } else {
        // NORMAL LOGIC: Warning color for HIGH values
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
    }

    uint8_t normalizedValue = static_cast<uint8_t>(normalizedPercent);

    return getContext().getColors().getColorFromPercent(normalizedValue, useDimColors_);
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

void MetricWidget::setReverseThresholds(bool reverse) {
    if (reverseThresholds_ != reverse) {
        reverseThresholds_ = reverse;
        markDirty();
    }
}

void MetricWidget::setUseDimColors(bool useDim) {
    if (useDimColors_ != useDim) {
        useDimColors_ = useDim;
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

void MetricWidget::setValueFormat(ValueFormat format) {
    if (valueFormat_ != format) {
        valueFormat_ = format;
        formatCacheDirty_ = true;
        markDirty();
    }
}

void MetricWidget::setTextSize(uint8_t size) {
    if (textSize_ != size) {
        textSize_ = size;
        markDirty();
    }
}

void MetricWidget::forceRefresh() {
    lastDrawnValue_ = -1;      // Force redraw by making last value different
    formatCacheDirty_ = true;  // Force format recalculation
    valueAreaDirty_ = true;    // Force area redraw
    markDirty();               // Mark widget as dirty

    // DEBUG: Force immediate draw if initialized
    if (isInitialized_ && getLcd()) {
        onDraw(true);
    }
}