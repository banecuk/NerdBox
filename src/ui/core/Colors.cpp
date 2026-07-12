#include "Colors.h"

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
    const uint16_t blue = 0x09EA;    // RGB(0, 62, 255)
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
    }
}

uint16_t Colors::getColorFromPercentGpu(uint8_t value) {
    if (value > 99) value = 99;

    // GPU gradient: dark red (idle) → mid red → deep red → bright alert red.
    // Pure red hue throughout (no green/blue channel) so it never drifts
    // into the brownish/olive territory a warm-grey/amber ramp produces.
    // Idle stays dark enough to be clearly distinct from the bright red
    // used at heavy load.
    const uint16_t darkRed = 0x2800;  // RGB( 40,   0,   0) — idle, near-black red
    const uint16_t midRed  = 0x5000;  // RGB( 82,   0,   0)
    const uint16_t deepRed = 0x8800;  // RGB(140,   0,   0)
    const uint16_t red     = 0xF800;  // RGB(255,   0,   0) — bright alert red

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 40) {
        C1 = darkRed; C2 = midRed;
        alpha = (value * 255) / 39;
    } else if (value < 70) {
        C1 = midRed; C2 = deepRed;
        alpha = ((value - 40) * 255) / 29;
    } else {
        C1 = deepRed; C2 = red;
        alpha = ((value - 70) * 255) / 29;
    }

    return blendRgb565(C1, C2, alpha);
}

uint16_t Colors::getColorFromPercentRam(uint8_t value) {
    if (value > 99) value = 99;

    // RAM gradient: dark slate (idle) → steel blue → muted bright blue
    // (high load). Desaturated (grey-blended) blue rather than a pure blue
    // channel, so it reads as a cool-toned cousin of the panel's grey/hairline
    // chrome instead of a saturated neon blue.
    const uint16_t darkSlate  = 0x10C5;  // RGB( 24,  24,  41) — idle, near-black slate
    const uint16_t steelBlue  = 0x322D;  // RGB( 48,  70, 107)
    const uint16_t brightBlue = 0x5C17;  // RGB( 90, 129, 189) — muted alert blue

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 50) {
        C1 = darkSlate; C2 = steelBlue;
        alpha = (value * 255) / 49;
    } else {
        C1 = steelBlue; C2 = brightBlue;
        alpha = ((value - 50) * 255) / 49;
    }

    return blendRgb565(C1, C2, alpha);
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