#pragma once

#include <cstdint>

// Pure value-area geometry for MetricWidget, pulled out of the widget so the
// alignment and clear-on-shift arithmetic is host-tested
// (test/MetricTileLayoutTest.cpp). Off-by-ones here used to show up only on
// hardware, as leftover glyph pixels or a value nudged out of its tile.
// No LGFX dependency: the widget maps its LovyanGFX text datum to HAlign.
namespace MetricTileLayout {

struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

enum class HAlign : uint8_t {
    Left,
    Center,
    Right
};

// The area the value (+ unit) is drawn in, inside a tile. A labelled tile
// puts the label column (labelWidth) and a separator on the left, and keeps
// only the right border margin; an unlabelled tile has a margin on both sides.
// Vertically, both keep borderMargin top and bottom.
inline Rect valueArea(const Rect& tile, bool hasLabel, int16_t labelWidth, int16_t separatorWidth,
                      int16_t borderMargin) {
    Rect area;
    if (hasLabel) {
        area.x = static_cast<int16_t>(tile.x + labelWidth + separatorWidth);
        area.w = static_cast<int16_t>(tile.w - labelWidth - separatorWidth - borderMargin);
    } else {
        area.x = static_cast<int16_t>(tile.x + borderMargin);
        area.w = static_cast<int16_t>(tile.w - 2 * borderMargin);
    }
    area.y = static_cast<int16_t>(tile.y + borderMargin);
    area.h = static_cast<int16_t>(tile.h - 2 * borderMargin);
    return area;
}

// Left x of the [value][unit] block of combined width totalW. Left/right
// alignment insets by textMargin; centred uses the same integer halving the
// widget always used (areaW/2 - totalW/2), so pixel positions don't move.
inline int16_t startX(int16_t areaX, int16_t areaW, int16_t totalW, HAlign align,
                      int16_t textMargin) {
    switch (align) {
        case HAlign::Left:
            return static_cast<int16_t>(areaX + textMargin);
        case HAlign::Right:
            return static_cast<int16_t>(areaX + areaW - textMargin - totalW);
        case HAlign::Center:
        default:
            return static_cast<int16_t>(areaX + areaW / 2 - totalW / 2);
    }
}

// Whether the old glyphs must be cleared before drawing a value of width
// newTextW where the last one was lastTextW. Without a unit, the per-glyph
// background fill overwrites a same-or-wider value, so only shrinking needs a
// clear. With a unit, the unit's x follows the value's width, so any change
// would leave the unit's old pixels behind.
inline bool needsClear(int16_t newTextW, int16_t lastTextW, int16_t unitW) {
    return unitW > 0 ? (newTextW != lastTextW) : (newTextW < lastTextW);
}

}  // namespace MetricTileLayout
