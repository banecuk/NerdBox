#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>

// Pure rate-text helpers shared by DiskInfoWidget and WidgetPainter's
// drawRateRow(), pulled out so the rounding/splitting is host-tested
// (test/RateFormatTest.cpp). No LGFX dependency.
namespace RateFormat {

// KB/s → whole MB/s, rounded half up. Negative input reads as 0.
inline int kbToWholeMb(float kbPerSec) {
    if (!(kbPerSec > 0.0f)) {
        return 0;
    }
    return static_cast<int>(kbPerSec / 1024.0f + 0.5f);
}

inline void formatWholeMb(char* buf, size_t len, float kbPerSec) {
    snprintf(buf, len, "%d", kbToWholeMb(kbPerSec));
}

// Formats value with one decimal and splits it into an integer part ("12")
// and a decimal part including the dot (".5"), so the two can be drawn in
// different fonts. hasData == false yields "--" and "". The decimal part
// keeps the dot even when it's ".0".
inline void splitOneDecimal(float value, bool hasData, char* intBuf, size_t intLen, char* decBuf,
                            size_t decLen) {
    if (!hasData) {
        snprintf(intBuf, intLen, "--");
        if (decLen > 0) {
            decBuf[0] = '\0';
        }
        return;
    }
    char full[16];
    snprintf(full, sizeof(full), "%.1f", static_cast<double>(value));
    char* dot = strchr(full, '.');
    if (dot) {
        *dot = '\0';
        snprintf(intBuf, intLen, "%s", full);
        snprintf(decBuf, decLen, ".%s", dot + 1);
    } else {
        snprintf(intBuf, intLen, "%s", full);
        if (decLen > 0) {
            decBuf[0] = '\0';
        }
    }
}

}  // namespace RateFormat
