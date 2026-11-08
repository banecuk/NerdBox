#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Pure now-playing text/geometry helpers for AudioWidget, pulled out so the
// quality-label rules and the unknown-length (B30) case are host-tested
// (test/AudioFormatTest.cpp). No LGFX dependency.
namespace AudioFormat {

// "m:ss" (minutes not capped, so a 75-minute track reads "75:00").
inline void formatTime(uint32_t ms, char* buf, size_t bufSize) {
    const uint32_t totalSeconds = ms / 1000;
    snprintf(buf, bufSize, "%lu:%02lu", static_cast<unsigned long>(totalSeconds / 60),
             static_cast<unsigned long>(totalSeconds % 60));
}

// Lossless formats are shown by name alone ("FLAC"); lossy ones pair the
// upper-cased format with its bitrate ("MP3 256"). Empty format → "".
inline bool isLossless(const char* formatUpper) {
    return strcmp(formatUpper, "FLAC") == 0 || strcmp(formatUpper, "WAV") == 0 ||
           strcmp(formatUpper, "APE") == 0 || strcmp(formatUpper, "ALAC") == 0;
}

inline void formatQuality(const char* format, const char* bitrate, char* buf, size_t bufSize) {
    if (bufSize == 0) {
        return;
    }
    char upper[16];
    size_t i = 0;
    for (; format != nullptr && format[i] != '\0' && i + 1 < sizeof(upper); ++i) {
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(format[i])));
    }
    upper[i] = '\0';

    if (upper[0] == '\0') {
        buf[0] = '\0';
        return;
    }
    if (!isLossless(upper) && bitrate != nullptr && bitrate[0] != '\0') {
        // Truncation is acceptable (the label is clipped on screen anyway);
        // checking the result tells GCC that, silencing -Wformat-truncation.
        if (snprintf(buf, bufSize, "%s %s", upper, bitrate) < 0) {
            buf[0] = '\0';
        }
    } else {
        snprintf(buf, bufSize, "%s", upper);
    }
}

// Position clamped to a known duration. durationMs == 0 means "unknown length"
// (a live stream), not "already at the end", so position passes through.
inline uint32_t clampPosition(uint32_t positionMs, uint32_t durationMs) {
    return (durationMs > 0 && positionMs > durationMs) ? durationMs : positionMs;
}

// "elapsed / total", or just "elapsed" for an unknown-length track (B30):
// "1:23 / 0:00" read as a bug.
inline void formatTimeLine(uint32_t positionMs, uint32_t durationMs, char* buf, size_t bufSize) {
    char elapsed[16];
    formatTime(clampPosition(positionMs, durationMs), elapsed, sizeof(elapsed));
    if (durationMs == 0) {
        snprintf(buf, bufSize, "%s", elapsed);
        return;
    }
    char total[16];
    formatTime(durationMs, total, sizeof(total));
    snprintf(buf, bufSize, "%s / %s", elapsed, total);
}

// Progress-bar fill in pixels; 0 for an unknown length or a zero-width bar.
inline uint16_t progressFillWidth(uint32_t positionMs, uint32_t durationMs, uint16_t barW) {
    if (durationMs == 0 || barW == 0) {
        return 0;
    }
    const uint32_t pos = clampPosition(positionMs, durationMs);
    return static_cast<uint16_t>((static_cast<uint64_t>(pos) * barW) / durationMs);
}

}  // namespace AudioFormat
