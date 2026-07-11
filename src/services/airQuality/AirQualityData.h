#pragma once

#include <Arduino.h>

// Minimal subset of the AirVisual API response.
// Only fields that are displayed on screen are stored here.
struct AirQualityData {
    bool          is_available = false;
    unsigned long last_update  = 0;    // millis() timestamp of last successful fetch

    // Weather
    int8_t   temperature    = 0;   // tp        — °C
    uint8_t  humidity       = 0;   // hu        — %
    int16_t  pressure       = 0;   // pr        — hPa
    uint16_t wind_speed_x10 = 0;   // ws*10     — m/s stored as integer (194 = 1.94)
    uint16_t wind_dir       = 0;   // wd        — degrees (0=N, 90=E, 180=S, 270=W)

    // Weather icon code ("01d", "10n", etc.) — max 3 chars + null terminator
    char icon_code[4] = {0};

    // Pollution
    uint16_t aqi_us = 0;            // aqius — US AQI (0–500)
};
