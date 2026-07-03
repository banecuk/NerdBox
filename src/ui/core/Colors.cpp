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

    // GPU gradient: warm near-black → warm grey → amber → dark red
    // Complements the orange (0xFD20) label colour; stays dark enough
    // for white text to be legible at all load levels.
    const uint16_t warmBlack = 0x2102;  // RGB( 32,  32,  16) — near-black, faint warm tint
    const uint16_t warmGrey  = 0x4A43;  // RGB( 72,  72,  24) — medium warm grey
    const uint16_t amber     = 0xA340;  // RGB(160, 104,   0) — dark amber
    const uint16_t red       = 0x7800;  // RGB(120,   0,   0) — dark red

    uint16_t C1, C2;
    uint8_t alpha;

    if (value < 40) {
        C1 = warmBlack; C2 = warmGrey;
        alpha = (value * 255) / 39;
    } else if (value < 70) {
        C1 = warmGrey; C2 = amber;
        alpha = ((value - 40) * 255) / 29;
    } else {
        C1 = amber; C2 = red;
        alpha = ((value - 70) * 255) / 29;
    }

    return blendRgb565(C1, C2, alpha);
}

#define MAKE_RGB565(r, g, b) (((r) << 11) | ((g) << 5) | (b))

uint16_t Colors::blendRgb565(uint16_t a, uint16_t b, uint8_t alpha) {
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