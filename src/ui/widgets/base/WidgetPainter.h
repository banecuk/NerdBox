#pragma once

#include <Arduino.h>

#include <cstdio>
#include <cstring>

#include "ui/resources/FontRegistry.h"

// Free-function draw helpers shared by a minority of widget subclasses
// (label-row-above-value/track layouts and tappable pill tracks). Extracted
// from the Widget base — most widgets don't use either — so Widget itself
// doesn't carry rendering helpers only some subclasses need.
namespace WidgetPainter {

// Small grey caption drawn top-left of the widget (e.g. "UPTIME",
// "BRIGHTNESS"), used by every widget with a label-row-above-value/track
// layout. Caller must already have painted the background.
inline void drawCaptionLabel(LGFX* lcd, int32_t x, int32_t y, const char* label,
                             uint16_t bgColor = TFT_BLACK) {
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, bgColor);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString(label, x, y + 2);
    Fonts::unload(lcd);
}

// Rounded-rect "pill" with a centered label, used by tappable
// switch/segment widgets (SwitchWidget, BrightnessWidget) for their
// on/off or active/inactive track.
inline void drawPillToggle(LGFX* lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint8_t radius, uint16_t bgColor, uint16_t textColor,
                           const char* label) {
    lcd->fillRoundRect(x, y, w, h, radius, bgColor);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(textColor, bgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(label, static_cast<int32_t>(x + w / 2), static_cast<int32_t>(y + h / 2));
    Fonts::unload(lcd);
}

// Draws a value followed by its unit, centred as a pair at (centerX,
// centerY). The value uses Fonts::loadMetric() in valueColor; the unit uses
// Fonts::loadLabel() in unitColor. A null/empty unit draws just the value,
// MC_DATUM-centred at the same point. Extracted from AirQualityWidget so
// RoomClimateWidget's matching value/unit column renders byte-identically
// rather than drifting from a second hand-copy.
//
// Caller is responsible for clearing the cell first — this only draws, since
// callers' clear rects don't always match the text's centring point (see
// AirQualityWidget::clearCell vs. rowCenterY).
inline void drawValueWithUnit(LGFX* lcd, int16_t centerX, int16_t centerY, const char* value,
                              const char* unit, uint16_t valueColor, uint16_t unitColor) {
    if (!unit || unit[0] == '\0') {
        // Baseline-anchored, not MC_DATUM: MC_DATUM's vertical centring is
        // font-metric-dependent and doesn't line up with the paired
        // value+unit path below, which always centres via this same
        // fontHeight/2 baseline offset. Using MC_DATUM here made
        // unit-less cells (e.g. the AQI value) sit visibly higher than
        // cells with a unit.
        Fonts::loadMetric(lcd);
        const int16_t valueH = static_cast<int16_t>(lcd->fontHeight());
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(C_BASELINE);
        lcd->drawString(value, centerX, centerY + valueH / 2);
        Fonts::unload(lcd);
        return;
    }

    int16_t valueW, valueH, unitW;
    Fonts::loadMetric(lcd);
    valueW = static_cast<int16_t>(lcd->textWidth(value));
    valueH = static_cast<int16_t>(lcd->fontHeight());
    Fonts::unload(lcd);
    Fonts::loadLabel(lcd);
    unitW = static_cast<int16_t>(lcd->textWidth(unit));
    Fonts::unload(lcd);

    const int16_t startX = centerX - (valueW + unitW) / 2;
    // Draw both on the same baseline so the unit sits at the bottom of the
    // value digits. The baseline of an MC_DATUM draw is ~half the value's own
    // glyph height below its vertical centre.
    const int16_t baselineY = centerY + valueH / 2;

    Fonts::loadMetric(lcd);
    lcd->setTextColor(valueColor, TFT_BLACK);
    lcd->setTextDatum(L_BASELINE);
    lcd->drawString(value, startX, baselineY);
    Fonts::unload(lcd);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(unitColor, TFT_BLACK);
    lcd->setTextDatum(L_BASELINE);
    lcd->drawString(unit, startX + valueW, baselineY);
    Fonts::unload(lcd);
}

// Shared by NetworkTrafficWidget and DiskSummaryWidget's stacked-rate-row
// layout (see docs-local/12-code-architecture.md, C2). Both split a rate
// value into right-aligned-integer / left-aligned-decimal fields on a fixed
// column so the decimal point doesn't drift as the digit count changes, then
// draw a direction indicator (arrow / letter) after it — but that indicator
// differs enough between the two widgets (a fillTriangle vs. a coloured
// letter) that it stays the caller's job.
struct RateRow {
    int16_t x, y, w, h;
    float value;      // already in display units (MB/s, Mbps, ...)
    bool hasData;     // false draws "--" instead of a formatted value
    uint8_t intDigits;  // 3 ("999") or 4 ("9999") — sizes the fixed integer column
    uint16_t valueColor;
    // Cache of the last *rendered* text + colour, in/out: a value that rounds
    // to the same displayed text doesn't force a redraw. Empty lastText
    // forces the first draw.
    char* lastText;
    size_t lastTextSize;
    uint16_t* lastColor;
    bool forceRedraw;
};

// Draws one rate row: fillRect the row black, then the formatted value in
// row.valueColor, skipping the redraw entirely when the rendered text+colour
// match the cache (unless forceRedraw). Returns the x column just past the
// value field, so the caller draws its own suffix (arrow / letter) at a
// position that never depends on the value string's measured width — or -1
// if the row was skipped, meaning the caller must not touch the suffix
// either (it's already showing the unchanged value from last time).
inline int16_t drawRateRow(LGFX* lcd, const RateRow& row) {
    char intBuf[16];
    char decBuf[4];
    if (!row.hasData) {
        snprintf(intBuf, sizeof(intBuf), "--");
        decBuf[0] = '\0';
    } else {
        char full[16];
        snprintf(full, sizeof(full), "%.1f", static_cast<double>(row.value));
        char* dot = strchr(full, '.');
        if (dot) {
            *dot = '\0';
            snprintf(intBuf, sizeof(intBuf), "%s", full);
            snprintf(decBuf, sizeof(decBuf), ".%s", dot + 1);
        } else {
            snprintf(intBuf, sizeof(intBuf), "%s", full);
            decBuf[0] = '\0';
        }
    }

    char buf[20];
    snprintf(buf, sizeof(buf), "%s%s", intBuf, decBuf);

    if (!row.forceRedraw && row.valueColor == *row.lastColor &&
        strncmp(buf, row.lastText, row.lastTextSize) == 0) {
        return -1;
    }

    lcd->fillRect(row.x, row.y, row.w, row.h, TFT_BLACK);

    const int16_t textY = row.y + row.h / 2;
    const int16_t textX = row.x + 2;

    Fonts::loadValue(lcd);
    lcd->setTextColor(row.valueColor, TFT_BLACK);

    const int16_t intFieldWidth = lcd->textWidth(row.intDigits >= 4 ? "9999" : "999");
    const int16_t decFieldWidth = lcd->textWidth(".9");
    const int16_t intColX = textX + intFieldWidth;

    lcd->setTextDatum(MR_DATUM);
    lcd->drawString(intBuf, intColX, textY);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(decBuf, intColX, textY);
    Fonts::unload(lcd);

    strncpy(row.lastText, buf, row.lastTextSize - 1);
    row.lastText[row.lastTextSize - 1] = '\0';
    *row.lastColor = row.valueColor;

    return intColX + decFieldWidth;
}

}  // namespace WidgetPainter
