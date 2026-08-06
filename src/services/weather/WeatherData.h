#pragma once

#include <Arduino.h>
#include <time.h>

#include <atomic>

#include "config/AppConfig.h"

// Minimal subset of the open-meteo daily forecast response.
// Fixed-size array, no heap. Field names are *10 scaled integers so the
// widget can format °C / mm / m/s with a single decimal point.
struct WeatherForecastDay {
    int16_t weatherCode;   // WMO weather code — drives the icon mapping
    int16_t tempMaxX10;    // °C * 10
    int16_t tempMinX10;    // °C * 10
    int16_t rainX10;       // mm * 10
    int16_t windMaxX10;    // m/s * 10
    time_t  dayStart;      // unixtime of local midnight (from daily.time[day])
};

struct WeatherData {
    bool is_available = false;
    unsigned long last_update = 0;   // millis() timestamp of last successful fetch

    WeatherForecastDay days[AppConfig::internal::WeatherImpl::kForecastDays];
    uint8_t dayCount = 0;

    // Written by the screen task and read by the background job — signals a
    // local-midnight rollover so the forecast refetches and shows the new day.
    std::atomic<bool> refreshRequested{false};
};