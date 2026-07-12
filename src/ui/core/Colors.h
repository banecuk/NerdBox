#pragma once

#include <Arduino.h>

#include <cstdint>

// static const byte COLOR_LEVELS_COUNT = 17;
// static const uint16_t COLOR_LEVELS[COLOR_LEVELS_COUNT] = { 0x09ea, 0x1229, 0x22c6,
// 0x3363, 0x3ba2, 0x4400, 0x5c40, 0x7460, 0x8ca0, 0x9CC0, 0xa460, 0xb400, 0xbba0, 0xCB20,
// 0xd280, 0xe180, 0xF800 };

class Colors {
 private:
    uint16_t COLOR_GRADIENT[100] = {};
    uint16_t COLOR_GRADIENT_DIM[100] = {};
    uint16_t generateColorFromPercent(byte value);
    void generateGradient();

 public:
    // Shared chrome constants — hairline borders/separators/idle states, and
    // inactive-label text, used across widgets so there's one place to tune
    // the look instead of scattered literals.
    static constexpr uint16_t kHairline = 0x2104;      // dark grey
    static constexpr uint16_t kInactiveText = 0x6B4D;  // mid-grey

    Colors();
    ~Colors();

    // Alpha-weighted blend between two RGB565 colors (alpha=0 -> a, alpha=255 -> b).
    static uint16_t blendRgb565(uint16_t a, uint16_t b, uint8_t alpha);

    uint16_t getColorFromPercent(uint8_t value, bool dim = false);
    uint16_t getColorFromPercentGpu(uint8_t value);
    uint16_t getColorFromPercentRam(uint8_t value);
    uint16_t darken(uint16_t color, uint8_t alpha);
};