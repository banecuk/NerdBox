#include "Colors.h"

#include "config/LgfxConfig.h"  // TFT_* color constants

uint16_t Colors::COLOR_GRADIENT[100] = {};
uint16_t Colors::COLOR_GRADIENT_DIM[100] = {};
uint16_t Colors::COLOR_GRADIENT_GPU[100] = {};
uint16_t Colors::COLOR_GRADIENT_RAM[100] = {};
uint16_t Colors::COLOR_GRADIENT_GRAY_GREEN[100] = {};

Colors::Colors() {
    generateGradient();
}

Colors::~Colors() {}

/* static */ uint16_t Colors::sampleRamp(const GradientStop* stops, size_t n, uint8_t value) {
    if (value <= stops[0].at)
        return stops[0].color;
    if (value >= stops[n - 1].at)
        return stops[n - 1].color;

    for (size_t i = 0; i + 1 < n; ++i) {
        if (value <= stops[i + 1].at) {
            const uint8_t alpha = static_cast<uint8_t>(
                ((value - stops[i].at) * 255) / (stops[i + 1].at - stops[i].at));
            return blendRgb565(stops[i].color, stops[i + 1].color, alpha);
        }
    }
    return stops[n - 1].color;  // unreachable given sorted stops
}

uint16_t Colors::getColorFromPercent(uint8_t value, bool dim) {
    if (value > 99) {
        value = 99;
    }
    return dim ? COLOR_GRADIENT_DIM[value] : COLOR_GRADIENT[value];
}

void Colors::generateGradient() {
    // CPU/default gradient: dark, muted blue (idle) -> green -> yellow-green
    // -> red (alert).
    static constexpr GradientStop kDefaultStops[] = {
        {0, 0x0947},   // dark, muted blue
        {25, 0x3BA2},  // green
        {60, 0x9CC0},  // yellow-green
        {99, 0xF800},  // red
    };

    // GPU gradient: dark red (idle) -> mid red -> deep red -> bright alert
    // red. Pure red hue throughout (no green/blue channel) so it never
    // drifts into brownish/olive territory, and idle stays dark enough to be
    // clearly distinct from the bright red used at heavy load.
    static constexpr GradientStop kGpuStops[] = {
        {0, 0x1041},   // idle, dark desaturated red
        {40, 0x5000},  // mid red
        {70, 0x8800},  // deep red
        {99, 0xF800},  // bright alert red
    };

    // RAM gradient: dark teal (idle) -> muted teal -> bright cyan (high
    // load). Teal keeps the cool blue family of the old slate/steel RAM ramp
    // but is far enough from the CPU's muted blue and the GPU's red to be
    // told apart at a glance.
    static constexpr GradientStop kRamStops[] = {
        {0, 0x08C3},   // idle, near-black teal
        {50, 0x11E7},  // low-moderate load
        {99, 0x4C71},  // bright alert cyan
    };

    // Light gray (low end of whatever scale the caller mapped its value
    // into) to light green (high end). Web "lightgrey" (0xD3D3D3) and
    // "lightgreen" (0x90EE90) in RGB565.
    static constexpr GradientStop kGrayGreenStops[] = {
        {0, 0xD6BA},
        {99, 0x9772},
    };

    for (int i = 0; i < 100; i++) {
        const uint8_t v = static_cast<uint8_t>(i);
        COLOR_GRADIENT[i] = sampleRamp(kDefaultStops, sizeof(kDefaultStops) / sizeof(GradientStop), v);
        COLOR_GRADIENT_DIM[i] = darken(COLOR_GRADIENT[i], 128);
        COLOR_GRADIENT_GPU[i] = sampleRamp(kGpuStops, sizeof(kGpuStops) / sizeof(GradientStop), v);
        COLOR_GRADIENT_RAM[i] = sampleRamp(kRamStops, sizeof(kRamStops) / sizeof(GradientStop), v);
        COLOR_GRADIENT_GRAY_GREEN[i] =
            sampleRamp(kGrayGreenStops, sizeof(kGrayGreenStops) / sizeof(GradientStop), v);
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

uint16_t Colors::getColorFromPercentGrayGreen(uint8_t value) {
    if (value > 99) {
        value = 99;
    }
    return COLOR_GRADIENT_GRAY_GREEN[value];
}

uint16_t Colors::utilizationColor(float percent) {
    if (percent < 60.0f) {
        // Smooth light-grey-to-light-green ramp across the idle-to-moderate
        // range, instead of an instant jump straight to full green the
        // moment utilisation ticks up from idle.
        const uint8_t idx = static_cast<uint8_t>(percent / 60.0f * 99.0f + 0.5f);
        return getColorFromPercentGrayGreen(idx);
    }
    if (percent < 85.0f)
        return kWarn;  // heavy
    if (percent < 100.0f)
        return kDanger;                    // near saturation
    return blendRgb565(kDanger, TFT_WHITE, 90);  // at/over the configured cap
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
