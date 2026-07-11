#pragma once

// FontRegistry — single include point for all five custom fonts.
//
// Usage in a widget:
//   #include "core/resources/FontRegistry.h"
//   Fonts::loadLabel(lcd);   // before drawString
//   Fonts::unload(lcd);      // immediately after drawString
//
// Always call unload() after every draw block. loadFont() streams the PROGMEM
// array into a runtime buffer; unload() releases it and restores the built-in
// bitmap font so other widgets aren't affected.
//
// Font roles
// ──────────────────────────────────────────────────────────────────────────
//  loadLabel()   NotoSansDisplay12         12 pt — small captions: "CPU",
//                                          "UPTIME", "STALE" badges, buttons
//
//  loadValue()   NotoSansDisplay15         15 pt — secondary values: IP
//                                          address, uptime HH:MM:SS
//
//  loadMetric()  NotoSans18                18 pt — primary metric values
//                                          inside MetricWidget cells
//
//  loadHeader()  NotoSansDisplayCondExt18  18 pt condensed — section
//                                          headings in narrow columns
//
//  loadMono()    NotoSansMono24            24 pt monospaced — ClockWidget
//                                          and FpsWidget digits
//
// Note on PROGMEM font headers
// ──────────────────────────────────────────────────────────────────────────
// Each font .h file is guarded by NERDBOX_DEFINE_FONT_DATA. Included here
// (that macro undefined), it expands to just `extern const uint8_t X[]
// PROGMEM;` — a declaration, not a definition, so no font data is duplicated
// into the TUs that include this registry. The one real definition of each
// array lives in Fonts.cpp, which defines NERDBOX_DEFINE_FONT_DATA before
// including the same headers. Every widget links against that single copy.

#include <Arduino.h>

#include "config/LgfxConfig.h"
#include "core/resources/NotoSans18.h"
#include "core/resources/NotoSansDisplay12.h"
#include "core/resources/NotoSansDisplay15.h"
#include "core/resources/NotoSansDisplayCondExt18.h"
#include "core/resources/NotoSansMono24.h"

struct Fonts {
    static void loadLabel(LGFX* lcd) {
        lcd->loadFont(NotoSansDisplay12);
        lcd->setTextSize(1);
    }

    static void loadValue(LGFX* lcd) {
        lcd->loadFont(NotoSansDisplay15);
        lcd->setTextSize(1);
    }

    static void loadMetric(LGFX* lcd) {
        lcd->loadFont(NotoSans18);
        lcd->setTextSize(1);
    }

    static void loadHeader(LGFX* lcd) {
        lcd->loadFont(NotoSansDisplayCondExt18);
        lcd->setTextSize(1);
    }

    static void loadMono(LGFX* lcd) {
        lcd->loadFont(NotoSansMono24);
        lcd->setTextSize(1);
    }

    static void unload(LGFX* lcd) {
        lcd->unloadFont();
    }
};
