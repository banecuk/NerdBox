#pragma once

#include <Arduino.h>

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
        Fonts::loadMetric(lcd);
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(value, centerX, centerY);
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

}  // namespace WidgetPainter
