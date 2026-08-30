#include "MetricWidget.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "ui/resources/FontRegistry.h"
#include "ui/widgets/base/MetricColorPolicy.h"

MetricWidget::MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           const Config& config)
    : Widget(dims, updateIntervalMs),
      value_(config.value),
      minValue_(config.minValue),
      maxValue_(config.maxValue),
      lowerThreshold_(std::min(config.lowerThreshold, config.upperThreshold)),
      upperThreshold_(std::max(config.lowerThreshold, config.upperThreshold)),
      reverseThresholds_(config.reverseThresholds),
      useDimColors_(config.useDimColors),
      useSmallFont_(config.useSmallFont),
      useGpuColors_(config.useGpuColors),
      useRamColors_(config.useRamColors),
      labelColor_(config.labelColor),
      labelWidth_(config.labelWidth),
      textAlignment_(config.textAlignment),
      borderMargin_(config.borderMargin),
      hasLabel_(config.label[0] != '\0'),
      verticalLabel_(config.verticalLabel),
      gradientBackground_(config.gradientBackground) {
    safeStringCopy(unit_, config.unit, sizeof(unit_));
    safeStringCopy(label_, config.label, sizeof(label_));
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

    if (valueChanged && hasDrawnOnce_ && !forceRedraw && !valueAreaDirty_ && !gradientBackground_) {
        // Optimization: only redraw text if just the value changed. Skipped
        // for gradientBackground_ tiles — a per-glyph opaque bg fill would
        // flatten the gradient into a solid block behind the new digits, so
        // those always retake the full background+text path below.
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

    int16_t areaX, areaY, areaWidth, areaHeight;
    getValueAreaBounds(areaX, areaY, areaWidth, areaHeight);

    // Clear value area with background color
    fillBackgroundArea(areaX, areaY, areaWidth, areaHeight, lastBgColor_);

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
    const uint16_t textBgColor = backgroundColorAtY(textY, areaY, areaHeight, lastBgColor_);

    drawValueText(displayText, startX, textY, textBgColor, gradientBackground_);
    unloadValueFont();

    if (unitW > 0) {
        Fonts::loadLabel(lcd);
        drawValueText(unit_, startX + valW, textY, textBgColor, gradientBackground_);
        Fonts::unload(lcd);
    }

    lastTextWidth_ = valW;
}

void MetricWidget::renderValueTextOnly() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    uint16_t newBgColor = calculateBackgroundColor();

    if (lastBgColor_ != newBgColor) {
        lastBgColor_ = newBgColor;
        renderValueArea();
        return;
    }

    const char* displayText = getFormattedValueText();
    const int16_t unitW = static_cast<int16_t>(unitWidthCache_);

    int16_t areaX, areaY, areaWidth, areaHeight;
    getValueAreaBounds(areaX, areaY, areaWidth, areaHeight);

    const int16_t textY = dimensions_.y + dimensions_.height / 2;
    const uint16_t textBgColor = backgroundColorAtY(textY, areaY, areaHeight, newBgColor);

    // Value font stays loaded across the measure, background clear, and draw
    // below — fewer setFont() calls, even though FontRegistry preloads every
    // font once at boot rather than streaming it from PROGMEM per call.
    loadValueFont();
    const int16_t newTextW = static_cast<int16_t>(lcd->textWidth(displayText));

    // When a unit is attached, its position depends on the value's width, so
    // ANY width change (not just shrinking) must clear the area to avoid
    // leaving stale unit pixels behind at the old position.
    const bool shifted = layoutShifted(newTextW, unitW);
    if (shifted) {
        fillBackgroundArea(areaX, areaY, areaWidth, areaHeight, newBgColor);
    }
    lastTextWidth_ = newTextW;

    const int16_t totalW = newTextW + unitW;
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);

    drawValueText(displayText, startX, textY, textBgColor);
    unloadValueFont();

    // The unit only needs to be repainted when the layout actually shifted —
    // otherwise the previously drawn glyphs are still valid on screen.
    if (shifted && unitW > 0) {
        Fonts::loadLabel(lcd);
        drawValueText(unit_, startX + newTextW, textY, textBgColor);
        Fonts::unload(lcd);
    }
}

void MetricWidget::drawValueWithLoadedFont() {
    // Fast path: the value font is already loaded by the caller's batch pass
    // — PcMetricsWidget::drawDynamicData() loads NotoSans18 (loadMetric) for
    // its tiles, DiskBandWidget::drawDynamicData() loads NotoSansDisplay15
    // (loadValue) for its small-font tiles. Either way this never calls
    // loadFont/unloadFont itself. The unit suffix (if any) is drawn separately
    // by drawUnitWithLoadedFont() in a follow-up pass under the label font.
    LGFX* lcd = getLcd();
    if (!lcd || !isStaticDrawn_)
        return;

    const uint16_t newBgColor = calculateBackgroundColor();

    // Unlike renderValueArea()/renderValueTextOnly() (reached via onDraw(),
    // which already gates on value/dirty changes before calling either), this
    // is called unconditionally once per tick by PcMetricsWidget's batch draw
    // — so it needs its own unchanged-value early-out.
    if (hasDrawnOnce_ && value_ == lastDrawnValue_ && newBgColor == lastBgColor_ && !isDirty() &&
        !valueAreaDirty_) {
        unitNeedsRedraw_ = false;
        return;
    }

    const char* displayText = getFormattedValueText();
    if (!displayText || displayText[0] == '\0')
        displayText = "0";
    const int16_t unitW = static_cast<int16_t>(unitWidthCache_);

    int16_t areaX, areaY, areaWidth, areaH;
    getValueAreaBounds(areaX, areaY, areaWidth, areaH);

    const int16_t textY = dimensions_.y + dimensions_.height / 2;
    const uint16_t textBgColor = backgroundColorAtY(textY, areaY, areaH, newBgColor);

    const int16_t newTextW = static_cast<int16_t>(lcd->textWidth(displayText));
    const bool bgChanged = (newBgColor != lastBgColor_);
    // When a unit is attached, its position depends on the value's width, so
    // ANY width change (not just shrinking) must clear the area — otherwise
    // stale unit pixels are left behind at the old position.
    const bool shifted = layoutShifted(newTextW, unitW);

    // A gradient background can't be preserved by a per-glyph opaque bg
    // fill (that would flatten the glyph's rows to one solid color instead
    // of the gradient sweeping behind it), so every redraw repaints the
    // whole gradient first and then draws the glyphs transparently on top.
    if (bgChanged || shifted || gradientBackground_) {
        fillBackgroundArea(areaX, areaY, areaWidth, areaH, newBgColor);
    }
    lastBgColor_ = newBgColor;
    lastTextWidth_ = newTextW;

    const int16_t totalW = newTextW + unitW;
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);

    // Per-glyph bg fill: setTextColor with bg param overwrites old digits in a
    // single pass — no blank frame, no flash, even after a background change.
    // Gradient tiles instead redraw the background above and then draw text
    // transparently, since the background was just freshly painted.
    drawValueText(displayText, startX, textY, textBgColor, gradientBackground_);

    // Stash the position for the paired drawUnitWithLoadedFont() call; only
    // flag it when the unit actually needs to move/recolour.
    unitDrawX_ = startX + newTextW;
    unitDrawY_ = textY;
    unitBgColor_ = textBgColor;
    unitNeedsRedraw_ = unitW > 0 && (bgChanged || shifted || gradientBackground_);

    lastDrawnValue_ = value_;
    hasDrawnOnce_ = true;
    clearDirty();
    valueAreaDirty_ = false;
}

void MetricWidget::drawUnitWithLoadedFont() {
    // Fast path: label font is already loaded by PcMetricsWidget's batch
    // caller (see drawDynamicData's second pass). No-op unless the paired
    // drawValueWithLoadedFont() call just moved or recoloured the unit.
    if (!unitNeedsRedraw_)
        return;

    if (!getLcd())
        return;

    drawValueText(unit_, unitDrawX_, unitDrawY_, unitBgColor_, gradientBackground_);

    unitNeedsRedraw_ = false;
}

void MetricWidget::loadValueFont() const {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    if (useSmallFont_) {
        Fonts::loadValue(lcd);
    } else {
        Fonts::loadMetric(lcd);
    }
}

void MetricWidget::unloadValueFont() const {
    LGFX* lcd = getLcd();
    if (lcd)
        Fonts::unload(lcd);
}

void MetricWidget::updateDimensionCache() {
    valueX_ =
        hasLabel_ ? dimensions_.x + labelWidth_ + SEPARATOR_WIDTH : dimensions_.x + borderMargin_;
    valueWidth_ = hasLabel_ ? dimensions_.width - labelWidth_ - SEPARATOR_WIDTH - borderMargin_
                            : dimensions_.width - (2 * borderMargin_);
    dimensionsDirty_ = false;
    valueAreaDirty_ = true;
}

const char* MetricWidget::getFormattedValueText() const {
    // The unit suffix is drawn separately (smaller, dimmed font) — see
    // refreshUnitWidthIfNeeded() — so this only formats the bare number.
    if (formatCacheDirty_) {
        snprintf(formattedValue_, sizeof(formattedValue_), "%d", value_);
        formatCacheDirty_ = false;
    }
    return formattedValue_;
}

void MetricWidget::refreshUnitWidthIfNeeded() const {
    if (!unitWidthDirty_)
        return;

    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    if (unit_[0] != '\0') {
        Fonts::loadLabel(lcd);
        unitWidthCache_ = static_cast<uint16_t>(lcd->textWidth(unit_));
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

    const uint8_t normalizedValue = MetricColorPolicy::normalizedPercent(
        value_, lowerThreshold_, upperThreshold_, reverseThresholds_);

    if (useGpuColors_) {
        return getContext().getColors().getColorFromPercentGpu(normalizedValue);
    }
    if (useRamColors_) {
        return getContext().getColors().getColorFromPercentRam(normalizedValue);
    }
    return getContext().getColors().getColorFromPercent(normalizedValue, useDimColors_);
}

void MetricWidget::fillBackgroundArea(int16_t x, int16_t y, int16_t w, int16_t h,
                                      uint16_t bottomColor) const {
    LGFX* lcd = getLcd();
    if (!lcd || w <= 0 || h <= 0)
        return;

    if (!gradientBackground_) {
        lcd->fillRect(x, y, w, h, bottomColor);
        return;
    }

    // One scanline per row, black at the top shading down to bottomColor —
    // only reached on a full-area redraw (background change or first
    // render), so the extra draw calls versus a single fillRect don't show
    // up on the per-tick hot path.
    for (int16_t row = 0; row < h; ++row) {
        lcd->drawFastHLine(x, y + row, w, backgroundColorAtY(y + row, y, h, bottomColor));
    }
}

uint16_t MetricWidget::backgroundColorAtY(int16_t y, int16_t areaY, int16_t areaHeight,
                                          uint16_t bottomColor) const {
    if (!gradientBackground_ || areaHeight <= 1)
        return bottomColor;

    const int16_t span = areaHeight - 1;
    int16_t offset = y - areaY;
    if (offset < 0)
        offset = 0;
    if (offset > span)
        offset = span;

    // RGB565: 5 bits R, 6 bits G, 5 bits B — interpolate each channel from 0
    // (pure black) to bottomColor's channel value.
    const uint8_t r2 = (bottomColor >> 11) & 0x1F;
    const uint8_t g2 = (bottomColor >> 5) & 0x3F;
    const uint8_t b2 = bottomColor & 0x1F;
    const uint8_t r = static_cast<uint8_t>((r2 * offset) / span);
    const uint8_t g = static_cast<uint8_t>((g2 * offset) / span);
    const uint8_t b = static_cast<uint8_t>((b2 * offset) / span);
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

void MetricWidget::getValueAreaBounds(int16_t& areaX, int16_t& areaY, int16_t& areaWidth,
                                      int16_t& areaHeight) const {
    if (hasLabel_) {
        areaX = valueX_;
        areaWidth = valueWidth_;
    } else {
        areaX = dimensions_.x + borderMargin_;
        areaWidth = dimensions_.width - (2 * borderMargin_);
    }
    areaY = dimensions_.y + borderMargin_;
    areaHeight = dimensions_.height - (2 * borderMargin_);
}

void MetricWidget::drawValueText(const char* text, int16_t x, int16_t y, uint16_t bgColor,
                                 bool transparent) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    if (transparent) {
        lcd->setTextColor(TFT_WHITE);
    } else {
        lcd->setTextColor(TFT_WHITE, bgColor);
    }
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(text, x, y);
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