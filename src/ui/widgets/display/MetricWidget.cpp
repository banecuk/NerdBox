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
      useSmallFont_(config.useSmallFont),
      palette_(config.palette),
      labelColor_(config.labelColor),
      labelWidth_(config.labelWidth),
      textAlignment_(config.textAlignment),
      borderMargin_(config.borderMargin),
      hasLabel_(config.label[0] != '\0'),
      verticalLabel_(config.verticalLabel),
      gradientBackground_(config.gradientBackground) {
    unit_.set(config.unit);
    label_.set(config.label);
    const MetricTileLayout::Rect tile{
        static_cast<int16_t>(dimensions_.x), static_cast<int16_t>(dimensions_.y),
        static_cast<int16_t>(dimensions_.width), static_cast<int16_t>(dimensions_.height)};
    valueArea_ = MetricTileLayout::valueArea(tile, hasLabel_, static_cast<int16_t>(labelWidth_),
                                             kSeparatorWidth, static_cast<int16_t>(borderMargin_));
}

void MetricWidget::onInitialize() {
    // Widget::initialize() runs this before its own first drawStatic(), so
    // the width is known before any draw path reads it.
    LGFX* lcd = getLcd();
    unitWidth_ = 0;
    if (lcd && unit_.c_str()[0] != '\0') {
        ScopedFont font(lcd, Fonts::loadLabel);
        unitWidth_ = static_cast<uint16_t>(lcd->textWidth(unit_.c_str()));
    }
}

void MetricWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

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
            const size_t len = strlen(label_.c_str());
            if (len > 0) {
                const int16_t rowH = dimensions_.height / static_cast<int16_t>(len);
                char ch[2] = {'\0', '\0'};
                for (size_t i = 0; i < len; ++i) {
                    ch[0] = label_.c_str()[i];
                    const int16_t labelY =
                        dimensions_.y + rowH * static_cast<int16_t>(i) + rowH / 2;
                    lcd->drawString(ch, labelX, labelY);
                }
            }
        } else {
            const int16_t labelY = dimensions_.y + (dimensions_.height / 2);
            lcd->drawString(label_.c_str(), labelX, labelY);
        }

        Fonts::unload(lcd);
    }

    lastTextWidth_ = 0;
    pending_ = RedrawLevel::Full;
}

void MetricWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_) {
        return;
    }

    if (pending_ == RedrawLevel::None && !forceRedraw && !isDirty()) {
        return;  // Nothing to do
    }

    // Optimization: only redraw text if just the value changed. Skipped for
    // gradientBackground_ tiles — a per-glyph opaque bg fill would flatten
    // the gradient into a solid block behind the new digits, so those always
    // retake the full background+text path.
    const bool valueOnly = pending_ == RedrawLevel::Value && !forceRedraw && !gradientBackground_;
    renderValue(/*fontsLoadedByCaller=*/false, /*clearWholeArea=*/!valueOnly);
    pending_ = RedrawLevel::None;
    // Kept, unlike other widgets (A15): child tiles are also drawn through
    // drawValueWithLoadedFont()/forceRefreshAndDraw(), outside Widget::draw().
    clearDirty();
}

void MetricWidget::renderValue(bool fontsLoadedByCaller, bool clearWholeArea) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const uint16_t newBgColor = calculateBackgroundColor();
    const bool bgChanged = newBgColor != lastBgColor_;

    // The batch path (fontsLoadedByCaller) is invoked unconditionally once
    // per tick by PcMetricsWidget/DiskBandWidget's draw pass, so it needs its
    // own unchanged-value early-out; the onDraw() path already gated on
    // value/dirty changes before choosing which flags to call in with.
    if (fontsLoadedByCaller && pending_ == RedrawLevel::None && !bgChanged && !isDirty()) {
        return;
    }

    int16_t areaX, areaY, areaWidth, areaHeight;
    getValueAreaBounds(areaX, areaY, areaWidth, areaHeight);

    // Bare number only — the unit suffix is drawn separately (smaller,
    // dimmed font). Formatted here, on the draw path only, rather than
    // cached: renderValue() runs only when something changed.
    char displayText[12];
    snprintf(displayText, sizeof(displayText), "%d", value_);
    const int16_t unitW = static_cast<int16_t>(unitWidth_);

    // Value font stays loaded across the measure and draw below — fewer
    // setFont() calls. Skipped when the caller already loaded it for the
    // whole tile batch.
    if (!fontsLoadedByCaller)
        loadValueFont();
    const int16_t newTextW = static_cast<int16_t>(lcd->textWidth(displayText));

    // When a unit is attached, its position depends on the value's width, so
    // ANY width change (not just shrinking) must clear the area — otherwise
    // stale unit pixels are left behind at the old position.
    const bool shifted = layoutShifted(newTextW, unitW);
    // A gradient background can't be preserved by a per-glyph opaque bg fill
    // (that would flatten it into one solid colour behind the new digits),
    // so it always retakes the full clear-then-redraw path.
    const bool clearNow = clearWholeArea || bgChanged || shifted || gradientBackground_;
    if (clearNow) {
        fillBackgroundArea(areaX, areaY, areaWidth, areaHeight, newBgColor);
    }
    lastBgColor_ = newBgColor;
    lastTextWidth_ = newTextW;

    const int16_t textY = dimensions_.y + dimensions_.height / 2;
    const uint16_t textBgColor = backgroundColorAtY(textY, areaY, areaHeight, newBgColor);
    const int16_t totalW = newTextW + unitW;
    const int16_t startX = computeStartX(areaX, areaWidth, totalW);

    // Per-glyph bg fill (transparent=false) overwrites old digits in a single
    // pass — no blank frame, no flash. Gradient tiles instead redraw the
    // background above and draw transparently on top of it. bold=!useSmallFont_
    // gives the primary tile grid's values a bit more visual weight than
    // their labels/units without changing size (see MetricWidget.h).
    drawValueText(displayText, startX, textY, textBgColor, gradientBackground_, !useSmallFont_);
    if (!fontsLoadedByCaller)
        unloadValueFont();

    if (fontsLoadedByCaller) {
        // Unit suffix is drawn separately by the caller's own follow-up pass
        // under the label font — see drawUnitWithLoadedFont(). Only flag it
        // when the unit actually needs to move/recolour.
        unitDrawX_ = startX + newTextW;
        unitDrawY_ = textY;
        unitBgColor_ = textBgColor;
        pending_ = (unitW > 0 && clearNow) ? RedrawLevel::Unit : RedrawLevel::None;
    } else if (unitW > 0 && clearNow) {
        // Only redrawn when the layout actually shifted (or this is a full
        // redraw) — otherwise the previously drawn glyphs are still valid.
        Fonts::loadLabel(lcd);
        drawValueText(unit_.c_str(), startX + newTextW, textY, textBgColor, gradientBackground_);
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
    if (!getLcd() || !isStaticDrawn_)
        return;

    // Leaves pending_ at Unit (unit suffix moved — drawUnitWithLoadedFont()
    // repaints it) or None.
    renderValue(/*fontsLoadedByCaller=*/true, /*clearWholeArea=*/false);
    if (pending_ != RedrawLevel::Unit)
        pending_ = RedrawLevel::None;
    clearDirty();
}

void MetricWidget::drawUnitWithLoadedFont() {
    // Fast path: label font is already loaded by PcMetricsWidget's batch
    // caller (see drawDynamicData's second pass). No-op unless the paired
    // drawValueWithLoadedFont() call just moved or recoloured the unit.
    if (pending_ != RedrawLevel::Unit)
        return;

    if (!getLcd())
        return;

    drawValueText(unit_.c_str(), unitDrawX_, unitDrawY_, unitBgColor_, gradientBackground_);

    pending_ = RedrawLevel::None;
}

void MetricWidget::loadValueFont() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    if (useSmallFont_) {
        Fonts::loadValue(lcd);
    } else {
        Fonts::loadMetric(lcd);
    }
}

void MetricWidget::unloadValueFont() {
    LGFX* lcd = getLcd();
    if (lcd)
        Fonts::unload(lcd);
}

uint16_t MetricWidget::calculateBackgroundColor() const {
    if (maxValue_ <= minValue_) {
        return TFT_BLACK;
    }

    const uint8_t normalizedValue = MetricColorPolicy::normalizedPercent(
        value_, lowerThreshold_, upperThreshold_, reverseThresholds_);

    switch (palette_) {
        case Palette::Gpu:
            return getContext().getColors().getColorFromPercentGpu(normalizedValue);
        case Palette::Ram:
            return getContext().getColors().getColorFromPercentRam(normalizedValue);
        case Palette::Dim:
            return getContext().getColors().getColorFromPercent(normalizedValue, /*dim=*/true);
        case Palette::Default:
        default:
            return getContext().getColors().getColorFromPercent(normalizedValue, /*dim=*/false);
    }
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
    // Computed once in the constructor; see MetricTileLayout::valueArea().
    areaX = valueArea_.x;
    areaY = valueArea_.y;
    areaWidth = valueArea_.w;
    areaHeight = valueArea_.h;
}

void MetricWidget::drawValueText(const char* text, int16_t x, int16_t y, uint16_t bgColor,
                                 bool transparent, bool bold) {
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
    if (bold) {
        // Faux-bold: a second pass 1px right thickens NotoSans18's regular
        // strokes just enough to read as heavier, without swapping in a true
        // bold face's different (and here, too-large) glyph metrics.
        //
        // The second pass must be transparent: drawn with a background colour
        // it repainted its whole glyph box and wiped out the first pass,
        // leaving the same weight shifted 1px (B54). On this non-readable
        // panel LovyanGFX's transparent mode skips empty pixels and blends
        // anti-aliased edges against the base colour, so point that at the
        // tile's background for the pass, then restore it.
        const uint32_t prevBase = lcd->getBaseColor();
        if (!transparent) {
            lcd->setBaseColor(bgColor);
        }
        lcd->setTextColor(TFT_WHITE);
        lcd->drawString(text, x + 1, y);
        lcd->setBaseColor(prevBase);
    }
}

bool MetricWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}

int16_t MetricWidget::computeStartX(int16_t areaX, int16_t areaWidth, int16_t totalW) const {
    MetricTileLayout::HAlign align = MetricTileLayout::HAlign::Center;
    if (textAlignment_ == ML_DATUM || textAlignment_ == CL_DATUM || textAlignment_ == BL_DATUM) {
        align = MetricTileLayout::HAlign::Left;
    } else if (textAlignment_ == MR_DATUM || textAlignment_ == CR_DATUM ||
               textAlignment_ == BR_DATUM) {
        align = MetricTileLayout::HAlign::Right;
    }
    return MetricTileLayout::startX(areaX, areaWidth, totalW, align, kTextMargin);
}

bool MetricWidget::layoutShifted(int16_t newTextWidth, int16_t unitWidth) const {
    return MetricTileLayout::needsClear(newTextWidth, lastTextWidth_, unitWidth);
}

void MetricWidget::setValue(int value) {
    if (value_ != value) {
        value_ = value;
        raise(RedrawLevel::Value);
        markDirty();
    }
}

void MetricWidget::forceRefreshAndDraw() {
    pending_ = RedrawLevel::Full;  // Force the next draw to take the full-render path
    markDirty();

    // Immediate draw — see the header comment for why this is safe here.
    if (isInitialized() && getLcd()) {
        onDraw(true);
    }
}