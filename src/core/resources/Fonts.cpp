// Fonts.cpp — single translation unit that owns all font array definitions.
//
// Each font header is guarded by NERDBOX_DEFINE_FONT_DATA: defined here (and
// ONLY here), it emits `extern const uint8_t FontName[] PROGMEM = { ... };`.
// Everywhere else (FontRegistry.h) the same header expands to just the
// `extern` declaration. That keeps the ~1.2 MB of font data compiled exactly
// once instead of duplicated into every translation unit that draws text.

#define NERDBOX_DEFINE_FONT_DATA

#include <Arduino.h>

#include "core/resources/NotoSans18.h"
#include "core/resources/NotoSansDisplay12.h"
#include "core/resources/NotoSansDisplay15.h"
#include "core/resources/NotoSansDisplayCondExt18.h"
#include "core/resources/NotoSansMono24.h"

#undef NERDBOX_DEFINE_FONT_DATA
