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

}  // namespace WidgetPainter
