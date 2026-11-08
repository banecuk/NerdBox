#pragma once

// FontRegistry — single include point for all five custom fonts.
//
// Usage in a widget:
//   #include "ui/resources/FontRegistry.h"
//   {
//       ScopedFont font(lcd, Fonts::loadLabel);  // loads now
//       lcd->drawString(...);
//   }                                            // unloads here, on every path
//
// Prefer ScopedFont over a hand-written loadX()/unload() pair in new code
// and anywhere with an early return between them (A17): the pairing then
// holds by construction instead of by audit. The bare loadX()/unload() calls
// remain for the batch paths whose contract is "caller has the font loaded"
// (MetricWidget::drawValueWithLoadedFont and its callers).
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
//
// MetricWidget's primary-value weight contrast (docs-local/03-visual-ux.md
// V4) is NOT a separate font here — two bundled LovyanGFX bold faces
// (Fonts/GFXFF) were tried and either overflowed the 28px tile rows (18pt)
// or still read as too heavy/large at a size that fit (12pt): GFXFF point
// sizes don't correspond to these VLW fonts'. MetricWidget instead draws its
// value text twice (see MetricWidget::drawValueText's `bold` param) — a
// faux-bold pass that thickens NotoSans18's own strokes without changing
// its glyph metrics.

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

// RAII font scope: loads a face on construction, Fonts::unload() on
// destruction. `load` is one of Fonts::loadLabel/loadValue/loadMetric/
// loadHeader/loadMono. use() switches face mid-scope; whichever face is
// current, the destructor still unloads.
class ScopedFont {
 public:
    using Loader = void (*)(LGFX*);

    ScopedFont(LGFX* lcd, Loader load) : lcd_(lcd) { load(lcd_); }
    ~ScopedFont() { Fonts::unload(lcd_); }

    void use(Loader load) { load(lcd_); }

    ScopedFont(const ScopedFont&) = delete;
    ScopedFont& operator=(const ScopedFont&) = delete;

 private:
    LGFX* lcd_;
};
