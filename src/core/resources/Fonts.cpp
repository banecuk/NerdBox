// Fonts.cpp — single translation unit that owns all font array definitions.
//
// Each font header defines its array as:
//   const uint8_t FontName[] PROGMEM = { ... };
//
// Including them here (and ONLY here) means the 1.2 MB of font data is
// compiled exactly once. Every other file that includes FontRegistry.h
// sees only the extern declarations and links against this object.

#include <Arduino.h>

#include "core/resources/NotoSans18.h"
#include "core/resources/NotoSansDisplay12.h"
#include "core/resources/NotoSansDisplay15.h"
#include "core/resources/NotoSansDisplayCondExt18.h"
#include "core/resources/NotoSansMono24.h"
