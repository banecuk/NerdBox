#pragma once

#include <cstdint>
#include <cstdio>

// Formatting/styling shared between WeatherWidget (full weather screen) and
// ForecastStripWidget (MultiWidget's compact candidate) — both render the
// same WeatherData, and this keeps their day names, rounding, and rain
// formatting byte-identical instead of two copies drifting apart.

// Abbreviated day names indexed by tm_wday (0 = Sunday).
static const char* const kDayNames[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static constexpr uint16_t kWeatherWeekendColor = 0xFBCF;  // light red for SAT/SUN day names
static constexpr uint16_t kWeatherRainColor = 0x867F;     // light blue, matches AirQuality humidity

// Converts a *10-scaled temperature to whole degrees, rounding to the nearest
// integer (half away from zero) instead of truncating — plain integer
// division on a negative X10 value truncates toward zero and displays up to
// 1° too warm.
static inline int16_t roundX10ToWhole(int16_t x10) {
    return static_cast<int16_t>(x10 >= 0 ? (x10 + 5) / 10 : (x10 - 5) / 10);
}

// Formats an X10-scaled value (rain mm, wind m/s) as "%d.%d" — one decimal
// place — into the caller's buffer.
static inline void formatX10OneDecimal(char* buf, size_t bufSize, int16_t x10) {
    snprintf(buf, bufSize, "%d.%d", x10 / 10, x10 % 10);
}
