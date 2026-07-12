#include "MetricWidget.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "core/resources/FontRegistry.h"

MetricWidget::MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs), useDimColors_(false) {
    updateDimensionCache();
    formattedValue_[0] = '\0';
}

void MetricWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    if (dimensionsDirty_) {
        updateDimensionCache();
    }

    refreshUnitWidthIfNeeded();

    // Clear the entire widget area
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Draw static elements (label if exists)
    if (hasLabel_ && labelWidth_ > 0) {
        const int16_t labelX = dimensions_.x + (labelWidth_ / 2);

        lcd->setTextColor(labelColor_, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        Fonts::loadLabel(lcd);

        if (verticalLabel_) {
            // Stack one character per row, evenly spaced across the tile's
            // full height — for tall (double-row) tiles where "RAM"/"VRAM"
            // would otherwise overflow the narrow label column.
            const size_t len = strlen(label_);
            if (len > 0) {
                const int16_t rowH = dimensions_.height / static_cast<int16_t>(len);
                char ch[2] = {'\0', '\0'};
                for (size_t i = 0; i < len; ++i) {
                    ch[0] = label_[i];
                    const int16_t labelY =
                        dimensions_.y + rowH * static_cast<int16_t>(i) + rowH / 2;
                    lcd->drawString(ch, labelX, labelY);
                }
            }
        } else {
            const int16_t labelY = dimensions_.y + (dimensions_.height / 2);
            lcd->drawString(label_, labelX, labelY);
        }

        Fonts::unload(lcd);
    }

    lastDrawnValue_ = -1;
    hasDrawnOnce_ = false;
    lastTextWidth_ = 0;
    valueAreaDirty_ = true;
}

void MetricWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_) {
        return;
    }

    refreshUnitWidthIfNeeded();

    bool valueChanged = (value_ != lastDrawnValue_);
    bool baseWidgetDirty = isDirty();

    // Always draw on the first render since drawStatic().
    bool firstRender = !hasDrawnOnce_;

    // Only check other dirty flags if value changed or forced
    if (!firstRender && !valueChanged && !forceRedraw && !baseWidgetDirty && !valueAreaDirty_) {
        return;  // Nothing to do
    }

    if (valueChanged && hasDrawnOnce_ && !forceRedraw && !valueAreaDirty_) {
        // Optimization: only redraw text if just the value changed
        renderValueTextOnly();
    } else {
        // Full value area redraw (background + text)
        renderValueArea();
    }
    lastDrawnValue_ = value_;
    hasDrawnOnce_ = true;
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

    loadValueFont();
    const int16_t valW = static_cast<int16_t>(lcd->textWidth(displayText));
    const int16_t unitW = static_cast<int16_t>(unitWidthCache_);
    const int16_t totalW = valW + unitW;

    // Calculate the combined [value][unit] block position based on alignment
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);
    const int16_t textY = dimensions_.y + dimensions_.height / 2;

    lcd->setTextColor(TFT_WHITE, lastBgColor_);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(displayText, startX, textY);
    unloadValueFont();

    if (unitW > 0) {
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_WHITE, lastBgColor_);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(getUnitText(), startX + valW, textY);
        Fonts::unload(lcd);
    }

    lastTextWidth_ = valW;
}

void MetricWidget::renderValueTextOnly() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    uint16_t newBgColor = calculateBackgroundColor();

    if (lastBgColor_ != newBgColor) {
        lastBgColor_ = newBgColor;
        renderValueArea();
        return;
    }

    const char* displayText = getFormattedValueText();
    const int16_t unitW = static_cast<int16_t>(unitWidthCache_);

    int16_t areaX, areaWidth;
    if (hasLabel_) {
        areaX    = valueX_;
        areaWidth = valueWidth_;
    } else {
        areaX    = dimensions_.x + BORDER_MARGIN;
        areaWidth = dimensions_.width - (2 * BORDER_MARGIN);
    }

    const int16_t textY = dimensions_.y + dimensions_.height / 2;

    // Value font stays loaded across the measure, background clear, and draw
    // below — loadFont()/unloadFont() stream from PROGMEM into a fresh heap
    // buffer each time, so this keeps it to one load instead of two.
    loadValueFont();
    const int16_t newTextW = static_cast<int16_t>(lcd->textWidth(displayText));

    // When a unit is attached, its position depends on the value's width, so
    // ANY width change (not just shrinking) must clear the area to avoid
    // leaving stale unit pixels behind at the old position.
    const bool shifted = layoutShifted(newTextW, unitW);
    if (shifted) {
        const int16_t areaY = dimensions_.y + BORDER_MARGIN;
        const int16_t areaH = dimensions_.height - (2 * BORDER_MARGIN);
        lcd->fillRect(areaX, areaY, areaWidth, areaH, newBgColor);
    }
    lastTextWidth_ = newTextW;

    const int16_t totalW = newTextW + unitW;
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);

    lcd->setTextColor(TFT_WHITE, newBgColor);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(displayText, startX, textY);
    unloadValueFont();

    // The unit only needs to be repainted when the layout actually shifted —
    // otherwise the previously drawn glyphs are still valid on screen.
    if (shifted && unitW > 0) {
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_WHITE, newBgColor);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(getUnitText(), startX + newTextW, textY);
        Fonts::unload(lcd);
    }
}

void MetricWidget::drawValueWithLoadedFont() {
    // Fast path: font is already loaded by PcMetricsWidget's batch caller.
    // We never call loadFont/unloadFont here — that's the caller's job. The
    // unit suffix (if any) is drawn separately by drawUnitWithLoadedFont() in
    // a follow-up pass under the label font — see PcMetricsWidget::drawDynamicData.
    //
    // Exception: small-font widgets use NotoSansDisplay15, not the batch font
    // (NotoSans18).  Fall back to renderValueTextOnly() which handles its own
    // load/unload (value + unit) so the batch font on the stack is left
    // undisturbed, and skip the paired drawUnitWithLoadedFont() call.
    if (useSmallFont_) {
        renderValueTextOnly();
        unitNeedsRedraw_ = false;
        return;
    }
    LGFX* lcd = getLcd();
    if (!lcd || !isStaticDrawn_) return;

    const uint16_t newBgColor = calculateBackgroundColor();
    const char* displayText = getFormattedValueText();
    if (!displayText || displayText[0] == '\0') displayText = "0";
    const int16_t unitW = static_cast<int16_t>(unitWidthCache_);

    int16_t areaX, areaWidth;
    if (hasLabel_) {
        areaX    = valueX_;
        areaWidth = valueWidth_;
    } else {
        areaX    = dimensions_.x + BORDER_MARGIN;
        areaWidth = dimensions_.width - (2 * BORDER_MARGIN);
    }

    const int16_t textY = dimensions_.y + dimensions_.height / 2;
    const int16_t areaY = dimensions_.y + BORDER_MARGIN;
    const int16_t areaH = dimensions_.height - (2 * BORDER_MARGIN);

    const int16_t newTextW = static_cast<int16_t>(lcd->textWidth(displayText));
    const bool bgChanged = (newBgColor != lastBgColor_);
    // When a unit is attached, its position depends on the value's width, so
    // ANY width change (not just shrinking) must clear the area — otherwise
    // stale unit pixels are left behind at the old position.
    const bool shifted = layoutShifted(newTextW, unitW);

    if (bgChanged || shifted) {
        lcd->fillRect(areaX, areaY, areaWidth, areaH, newBgColor);
    }
    lastBgColor_ = newBgColor;
    lastTextWidth_ = newTextW;

    const int16_t totalW = newTextW + unitW;
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);

    // Per-glyph bg fill: setTextColor with bg param overwrites old digits in a
    // single pass — no blank frame, no flash, even after a background change.
    lcd->setTextColor(TFT_WHITE, newBgColor);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(displayText, startX, textY);

    // Stash the position for the paired drawUnitWithLoadedFont() call; only
    // flag it when the unit actually needs to move/recolour.
    unitDrawX_ = startX + newTextW;
    unitDrawY_ = textY;
    unitBgColor_ = newBgColor;
    unitNeedsRedraw_ = unitW > 0 && (bgChanged || shifted);

    lastDrawnValue_ = value_;
    hasDrawnOnce_ = true;
    clearDirty();
}

void MetricWidget::drawUnitWithLoadedFont() {
    // Fast path: label font is already loaded by PcMetricsWidget's batch
    // caller (see drawDynamicData's second pass). No-op unless the paired
    // drawValueWithLoadedFont() call just moved or recoloured the unit.
    if (!unitNeedsRedraw_) return;

    LGFX* lcd = getLcd();
    if (!lcd) return;

    lcd->setTextColor(TFT_WHITE, unitBgColor_);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(getUnitText(), unitDrawX_, unitDrawY_);

    unitNeedsRedraw_ = false;
}


void MetricWidget::loadValueFont() const {
    LGFX* lcd = getLcd();
    if (!lcd) return;
    if (useSmallFont_) {
        Fonts::loadValue(lcd);
    } else {
        Fonts::loadMetric(lcd);
    }
}

void MetricWidget::unloadValueFont() const {
    LGFX* lcd = getLcd();
    if (lcd) Fonts::unload(lcd);
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
    // The unit suffix is drawn separately (smaller, dimmed font) — see
    // getUnitText() and refreshUnitWidthIfNeeded() — so this only formats
    // the bare number.
    if (formatCacheDirty_) {
        snprintf(formattedValue_, sizeof(formattedValue_), "%d", value_);
        formatCacheDirty_ = false;
    }
    return formattedValue_;
}

const char* MetricWidget::getUnitText() const {
    switch (valueFormat_) {
        case ValueFormat::kPercent:
            return "%";
        case ValueFormat::kRpm:
            return " RPM";
        case ValueFormat::kWatts:
            return "W";
        case ValueFormat::kCelsius:
            return "\xC2\xB0""C";
        case ValueFormat::kMB:
            return " MB";
        case ValueFormat::kDefault:
        default:
            return unit_;
    }
}

void MetricWidget::refreshUnitWidthIfNeeded() const {
    if (!unitWidthDirty_) return;

    LGFX* lcd = getLcd();
    if (!lcd) return;

    const char* unit = getUnitText();
    if (unit[0] != '\0') {
        Fonts::loadLabel(lcd);
        unitWidthCache_ = static_cast<uint16_t>(lcd->textWidth(unit));
        Fonts::unload(lcd);
    } else {
        unitWidthCache_ = 0;
    }
    unitWidthDirty_ = false;
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

    if (useGpuColors_) {
        return getContext().getColors().getColorFromPercentGpu(normalizedValue);
    }
    if (useRamColors_) {
        return getContext().getColors().getColorFromPercentRam(normalizedValue);
    }
    return getContext().getColors().getColorFromPercent(normalizedValue, useDimColors_);
}

bool MetricWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}

int16_t MetricWidget::computeStartX(int16_t areaX, int16_t areaWidth, int16_t totalW) const {
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        return areaX + TEXT_MARGIN;
    }
    if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM || textAlignment_ == BR_DATUM) {
        return areaX + areaWidth - TEXT_MARGIN - totalW;
    }
    return areaX + areaWidth / 2 - totalW / 2;
}

bool MetricWidget::layoutShifted(int16_t newTextWidth, int16_t unitWidth) const {
    return unitWidth > 0 ? (newTextWidth != lastTextWidth_) : (newTextWidth < lastTextWidth_);
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
        unitWidthDirty_ = true;
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

void MetricWidget::setUseSmallFont(bool small) {
    if (useSmallFont_ != small) {
        useSmallFont_ = small;
        valueAreaDirty_ = true;
        markDirty();
    }
}

void MetricWidget::setLabelColor(uint16_t color) {
    if (labelColor_ != color) {
        labelColor_ = color;
        markDirty();
    }
}

void MetricWidget::setUseGpuColors(bool use) {
    if (useGpuColors_ != use) {
        useGpuColors_ = use;
        markDirty();
    }
}

void MetricWidget::setUseRamColors(bool use) {
    if (useRamColors_ != use) {
        useRamColors_ = use;
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

void MetricWidget::setVerticalLabel(bool vertical) {
    if (verticalLabel_ != vertical) {
        verticalLabel_ = vertical;
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
        unitWidthDirty_ = true;
        markDirty();
    }
}

void MetricWidget::forceRefresh() {
    hasDrawnOnce_ = false;     // Force the next draw to take the full-render path
    formatCacheDirty_ = true;  // Force format recalculation
    valueAreaDirty_ = true;    // Force area redraw
    markDirty();               // Mark widget as dirty

    // DEBUG: Force immediate draw if initialized
    if (isInitialized_ && getLcd()) {
        onDraw(true);
    }
}