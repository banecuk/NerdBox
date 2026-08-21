#pragma once

#include <Arduino.h>

#include <cstdint>

// static const byte COLOR_LEVELS_COUNT = 17;
// static const uint16_t COLOR_LEVELS[COLOR_LEVELS_COUNT] = { 0x09ea, 0x1229, 0x22c6,
// 0x3363, 0x3ba2, 0x4400, 0x5c40, 0x7460, 0x8ca0, 0x9CC0, 0xa460, 0xb400, 0xbba0, 0xCB20,
// 0xd280, 0xe180, 0xF800 };

class Colors {
 private:
    // Static, not per-instance: Colors is only ever constructed once (as a
    // member of ApplicationComponents), so there's no reason for these 800
    // bytes of lookup tables to live inside every Colors object — keeping
    // them static keeps sizeof(Colors) (and therefore ApplicationComponents)
    // unchanged from before these tables existed.
    static uint16_t COLOR_GRADIENT[100];
    static uint16_t COLOR_GRADIENT_DIM[100];
    static uint16_t COLOR_GRADIENT_GPU[100];
    static uint16_t COLOR_GRADIENT_RAM[100];
    // Light-gray-to-light-green ramp shared by every widget that wants a
    // smooth "idle/low -> good/high" scale instead of a metric-specific hue
    // (CpuClockWidget's per-core MHz color, NetworkTrafficWidget's idle-to-
    // moderate utilisation color) — named for the ramp's colors, not either
    // caller's metric, since it isn't specific to either one.
    static uint16_t COLOR_GRADIENT_GRAY_GREEN[100];
    uint16_t generateColorFromPercent(byte value);
    uint16_t generateColorFromPercentGpu(byte value);
    uint16_t generateColorFromPercentRam(byte value);
    uint16_t generateColorFromPercentGrayGreen(byte value);
    void generateGradient();
    static uint16_t diskActivityColorScale(float kbPerSec, uint16_t darkColor,
                                            uint16_t brightColor);

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

    // Disk activity color scales, in KB/s: <1 MB/s dark gray (idle), then a
    // continuous blend from a dark to a bright shade of the scale's hue as
    // the rate climbs to a 100 MB/s cap (everything at/above stays at the
    // brightest shade). Read uses green shades, write uses red shades, so the
    // two directions stay visually distinguishable at a glance.
    static uint16_t diskReadActivityColor(float kbPerSec);
    static uint16_t diskWriteActivityColor(float kbPerSec);

    uint16_t getColorFromPercent(uint8_t value, bool dim = false);
    uint16_t getColorFromPercentGpu(uint8_t value);
    uint16_t getColorFromPercentRam(uint8_t value);
    // value is 0-99, mapping linearly onto whatever range the caller scaled
    // its raw metric into (see CpuClockWidget::clockPercent,
    // NetworkTrafficWidget::trafficColor) — the gradient itself doesn't know
    // what metric it's for, same as the other getColorFromPercent*
    // accessors; it just indexes a precomputed table instead of blending two
    // RGB565 colors on every draw.
    uint16_t getColorFromPercentGrayGreen(uint8_t value);
    uint16_t darken(uint16_t color, uint8_t alpha);
};