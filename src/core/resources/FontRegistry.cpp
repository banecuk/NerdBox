#include "core/resources/FontRegistry.h"

// Each font .h file is guarded by NERDBOX_DEFINE_FONT_DATA. Included here
// (that macro undefined), it expands to just `extern const uint8_t X[]
// PROGMEM;` — a declaration, not a definition, so no font data is duplicated
// into this TU. The one real definition of each array lives in Fonts.cpp,
// which defines NERDBOX_DEFINE_FONT_DATA before including the same headers.
#include "core/resources/NotoSans18.h"
#include "core/resources/NotoSansDisplay12.h"
#include "core/resources/NotoSansDisplay15.h"
#include "core/resources/NotoSansDisplayCondExt18.h"
#include "core/resources/NotoSansMono24.h"

namespace {

// Owns one parsed glyph table + the PROGMEM reader it was built from. Both
// must live for the process — VLWfont::drawChar reads glyph bitmaps back
// through the PointerWrapper on every draw.
struct FontSlot {
    lgfx::PointerWrapper data;
    lgfx::VLWfont font;

    void init(const uint8_t* progmem) {
        data.set(progmem);
        font.loadFont(&data);
    }
};

enum class FontId : size_t { kLabel, kValue, kMetric, kHeader, kMono, kCount };

FontSlot g_slots[static_cast<size_t>(FontId::kCount)];
bool g_initialized = false;

inline lgfx::VLWfont* slot(FontId id) {
    return &g_slots[static_cast<size_t>(id)].font;
}

}  // namespace

void Fonts::init() {
    if (g_initialized) return;

    g_slots[static_cast<size_t>(FontId::kLabel)].init(NotoSansDisplay12);
    g_slots[static_cast<size_t>(FontId::kValue)].init(NotoSansDisplay15);
    g_slots[static_cast<size_t>(FontId::kMetric)].init(NotoSans18);
    g_slots[static_cast<size_t>(FontId::kHeader)].init(NotoSansDisplayCondExt18);
    g_slots[static_cast<size_t>(FontId::kMono)].init(NotoSansMono24);

    g_initialized = true;
}

void Fonts::loadLabel(LGFX* lcd) {
    lcd->setFont(slot(FontId::kLabel));
    lcd->setTextSize(1);
}

void Fonts::loadValue(LGFX* lcd) {
    lcd->setFont(slot(FontId::kValue));
    lcd->setTextSize(1);
}

void Fonts::loadMetric(LGFX* lcd) {
    lcd->setFont(slot(FontId::kMetric));
    lcd->setTextSize(1);
}

void Fonts::loadHeader(LGFX* lcd) {
    lcd->setFont(slot(FontId::kHeader));
    lcd->setTextSize(1);
}

void Fonts::loadMono(LGFX* lcd) {
    lcd->setFont(slot(FontId::kMono));
    lcd->setTextSize(1);
}

void Fonts::unload(LGFX* lcd) {
    lcd->setFont(&lgfx::fonts::Font0);
}
