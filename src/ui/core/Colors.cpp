#include "Colors.h"

#include "config/LgfxConfig.h"  // TFT_* color constants

uint16_t Colors::COLOR_GRADIENT[100] = {};
uint16_t Colors::COLOR_GRADIENT_DIM[100] = {};
uint16_t Colors::COLOR_GRADIENT_GPU[100] = {};
uint16_t Colors::COLOR_GRADIENT_RAM[100] = {};

Colors::Colors() {
    generateGradient();
}

Colors::~Colors() {}

uint16_t Colors::getColorFromPercent(uint8_t value, bool dim) {
    if (value > 99) {
        value = 99;
    }
    return dim ? COLOR_GRADIENT_DIM[value] : COLOR_GRADIENT[value];
}

uint16_t Colors::generateColorFromPercent(uint8_t value) {
    // Define colors in RGB565 format
    const uint16_t blue = 0x0947;    // RGB(8, 40, 56) — dark, muted blue (base)
    const uint16_t green = 0x3BA2;   // RGB(7, 180, 2)
    const uint16_t yellow = 0x9CC0;  // RGB(19, 248, 0) - actually more green-yellow
    const uint16_t red = 0xF800;     // RGB(31, 0, 0)

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 25) {
        // Blue to Green: 0-24% (25 values)
        C1 = blue;
        C2 = green;
        alpha = (value * 255) / 24;  // Proper linear interpolation
    } else if (value < 60) {
        // Green to Yellow: 25-59% (35 values)
        C1 = green;
        C2 = yellow;
        alpha = ((value - 25) * 255) / 34;  // 59-25=34 range
    } else {
        // Yellow to Red: 60-99% (40 values)
        C1 = yellow;
        C2 = red;
        alpha = ((value - 60) * 255) / 39;  // 99-60=39 range
    }

    return blendRgb565(C1, C2, alpha);
}

void Colors::generateGradient() {
    for (int i = 0; i < 100; i++) {
        COLOR_GRADIENT[i] = generateColorFromPercent(i);
        COLOR_GRADIENT_DIM[i] = darken(COLOR_GRADIENT[i], 128);
        COLOR_GRADIENT_GPU[i] = generateColorFromPercentGpu(i);
        COLOR_GRADIENT_RAM[i] = generateColorFromPercentRam(i);
    }
}

uint16_t Colors::getColorFromPercentGpu(uint8_t value) {
    if (value > 99) {
        value = 99;
    }
    return COLOR_GRADIENT_GPU[value];
}

uint16_t Colors::getColorFromPercentRam(uint8_t value) {
    if (value > 99) {
        value = 99;
    }
    return COLOR_GRADIENT_RAM[value];
}

uint16_t Colors::generateColorFromPercentGpu(uint8_t value) {
    // GPU gradient: dark red (idle) → mid red → deep red → bright alert red.
    // Pure red hue throughout (no green/blue channel) so it never drifts
    // into the brownish/olive territory a warm-grey/amber ramp produces.
    // Idle stays dark enough to be clearly distinct from the bright red
    // used at heavy load.
    const uint16_t darkRed = 0x1041;  // RGB(16,  8,  8) — idle, dark desaturated red
    const uint16_t midRed = 0x5000;   // RGB(82,   0,   0)
    const uint16_t deepRed = 0x8800;  // RGB(140,   0,   0)
    const uint16_t red = 0xF800;      // RGB(255,   0,   0) — bright alert red

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 40) {
        C1 = darkRed;
        C2 = midRed;
        alpha = (value * 255) / 39;
    } else if (value < 70) {
        C1 = midRed;
        C2 = deepRed;
        alpha = ((value - 40) * 255) / 29;
    } else {
        C1 = deepRed;
        C2 = red;
        alpha = ((value - 70) * 255) / 29;
    }

    return blendRgb565(C1, C2, alpha);
}

uint16_t Colors::generateColorFromPercentRam(uint8_t value) {
    // RAM gradient: dark teal (idle) → muted teal → bright cyan (high load).
    // Teal keeps the cool blue family of the old slate/steel RAM ramp but is
    // far enough from the CPU's muted blue and the GPU's red to be told apart
    // at a glance. Idle stays near-black so it never reads as "active".
    const uint16_t darkTeal = 0x08C3;    // RGB(  8,  24,  25) — idle, near-black teal
    const uint16_t midTeal = 0x11E7;     // RGB( 16,  61,  58) — low-moderate load
    const uint16_t brightCyan = 0x4C71;  // RGB( 74, 142, 140) — bright alert cyan

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 50) {
        C1 = darkTeal;
        C2 = midTeal;
        alpha = (value * 255) / 49;
    } else {
        C1 = midTeal;
        C2 = brightCyan;
        alpha = ((value - 50) * 255) / 49;
    }

    return blendRgb565(C1, C2, alpha);
}

// Disk activity color scale, in KB/s: <1 MB/s dark gray (idle), then a
// continuous linear blend from darkColor to brightColor as the rate climbs
// from the idle threshold to a 100 MB/s cap (capped -- everything at/above
// stays at brightColor). The blend starts at the halfway point between
// darkColor and brightColor rather than at darkColor itself, so even minimal
// activity (just above idle) renders noticeably brighter and easier to spot.
/* static */ uint16_t Colors::diskActivityColorScale(float kbPerSec, uint16_t darkColor,
                                                      uint16_t brightColor) {
    constexpr float kIdle = 1.0f * 1024.0f;
    constexpr float kMax = 100.0f * 1024.0f;

    if (kbPerSec < kIdle)
        return kHairline;
    const float clamped = kbPerSec > kMax ? kMax : kbPerSec;
    const float t = (clamped - kIdle) / (kMax - kIdle);  // 0 at idle, 1 at kMax
    const uint8_t alpha = static_cast<uint8_t>((0.5f + 0.5f * t) * 255.0f + 0.5f);
    return blendRgb565(darkColor, brightColor, alpha);
}

/* static */ uint16_t Colors::diskReadActivityColor(float kbPerSec) {
    return diskActivityColorScale(kbPerSec, TFT_DARKGREEN, TFT_GREEN);
}

/* static */ uint16_t Colors::diskWriteActivityColor(float kbPerSec) {
    // TFT_DARKRED is a known upstream LovyanGFX/TFT_eSPI bug: it's defined as
    // 0x8B00, a byte-for-byte duplicate of TFT_DARKMAGENTA's value, which
    // actually decodes to an olive/dark-yellow-green RGB565 triplet rather
    // than dark red — blending toward it produced yellow/orange write-line
    // shades instead of a clean red gradient. TFT_MAROON (0x7800, a genuine
    // (128,0,0)) is the correct dark-red anchor instead.
    return diskActivityColorScale(kbPerSec, TFT_MAROON, TFT_RED);
}

#define MAKE_RGB565(r, g, b) (((r) << 11) | ((g) << 5) | (b))

/* static */ uint16_t Colors::blendRgb565(uint16_t a, uint16_t b, uint8_t alpha) {
    const uint16_t invAlpha = 255 - alpha;

    // Extract RGB components (5-6-5 format)
    uint16_t A_r = (a >> 11) & 0x1F;
    uint16_t A_g = (a >> 5) & 0x3F;
    uint16_t A_b = a & 0x1F;

    uint16_t B_r = (b >> 11) & 0x1F;
    uint16_t B_g = (b >> 5) & 0x3F;
    uint16_t B_b = b & 0x1F;

    // Blend each component
    uint16_t C_r = (A_r * invAlpha + B_r * alpha) / 255;
    uint16_t C_g = (A_g * invAlpha + B_g * alpha) / 255;
    uint16_t C_b = (A_b * invAlpha + B_b * alpha) / 255;

    return MAKE_RGB565(C_r, C_g, C_b);
}

uint16_t Colors::darken(uint16_t color, uint8_t factor) {
    // Convert RGB565 to RGB888 using integer math
    uint16_t r5 = (color >> 11) & 0x1F;
    uint16_t g6 = (color >> 5) & 0x3F;
    uint16_t b5 = color & 0x1F;

    uint16_t r = (r5 * 255 + 15) / 31;  // +15 for rounding
    uint16_t g = (g6 * 255 + 31) / 63;  // +31 for rounding
    uint16_t b = (b5 * 255 + 15) / 31;

    // Simple luminance-based darkening (approximates HSL)
    uint32_t luminance = (r * 299 + g * 587 + b * 114) / 1000;
    uint32_t new_luminance = (luminance * (255 - factor)) / 255;

    if (luminance == 0)
        return color;  // avoid division by zero

    // Scale RGB components proportionally to maintain hue
    r = (r * new_luminance) / luminance;
    g = (g * new_luminance) / luminance;
    b = (b * new_luminance) / luminance;

    // Convert back to RGB565
    uint8_t r5_out = (r * 31) / 255;
    uint8_t g6_out = (g * 63) / 255;
    uint8_t b5_out = (b * 31) / 255;

    return MAKE_RGB565(r5_out, g6_out, b5_out);
}