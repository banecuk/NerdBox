#pragma once

// FontRegistry — single include point for all five custom fonts.
//
// Usage in a widget:
//   #include "core/resources/FontRegistry.h"
//   Fonts::loadLabel(lcd);   // before drawString
//   Fonts::unload(lcd);      // immediately after drawString
//
// Fonts::init() parses every PROGMEM font array into a runtime glyph table
// exactly once (called from ApplicationComponents::initializeDisplay(), right
// after the display is up). Every loadX()/unload() call after that is just an
// LGFXBase::setFont() pointer swap — no allocation, no PROGMEM re-read. Call
// unload() after every draw block so other widgets aren't left drawing with
// the wrong font.
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

#include <Arduino.h>

#include "config/LgfxConfig.h"

struct Fonts {
    // Parses all five PROGMEM fonts into runtime glyph tables. Call once,
    // after the display is initialized. Idempotent.
    static void init();

    static void loadLabel(LGFX* lcd);
    static void loadValue(LGFX* lcd);
    static void loadMetric(LGFX* lcd);
    static void loadHeader(LGFX* lcd);
    static void loadMono(LGFX* lcd);

    static void unload(LGFX* lcd);
};
