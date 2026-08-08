#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <atomic>

#include "config/AppConfig.h"
#include "utils/PublishedFlag.h"

#include <time.h>

// Minimal subset of the open-meteo daily forecast response.
// Fixed-size array, no heap. Field names are *10 scaled integers so the
// widget can format °C / mm / m/s with a single decimal point.
struct WeatherForecastDay {
    int16_t weatherCode;  // WMO weather code — drives the icon mapping
    int16_t tempMaxX10;   // °C * 10
    int16_t tempMinX10;   // °C * 10
    int16_t rainX10;      // mm * 10
    int16_t windMaxX10;   // m/s * 10
    time_t dayStart;      // unixtime of local midnight (from daily.time[day])
};

struct WeatherData {
    WeatherData() = default;
    ~WeatherData() { vSemaphoreDelete(daysMutex); }

    // There is exactly one WeatherData instance for the app's lifetime.
    // Copying/moving it would duplicate/steal the SemaphoreHandle_t below.
    WeatherData(const WeatherData&) = delete;
    WeatherData& operator=(const WeatherData&) = delete;
    WeatherData(WeatherData&&) = delete;
    WeatherData& operator=(WeatherData&&) = delete;

    // freshness.publish() is the cross-core publish point — see
    // DataFreshnessGuard for the happens-before argument.
    PublishedFlag freshness;

    // Mutex protecting days[] only — the background fetch (background task)
    // and WeatherWidget's draw (screen task) both touch it. freshness /
    // dayCount / refreshRequested are word-sized and accessed as naturally
    // atomic, mirroring PcMetrics's disk_drives lock pattern.
    SemaphoreHandle_t daysMutex = xSemaphoreCreateMutex();

    WeatherForecastDay days[AppConfig::internal::WeatherImpl::kForecastDays];
    uint8_t dayCount = 0;

    // Written by the screen task and read by the background job — signals a
    // local-midnight rollover so the forecast refetches and shows the new day.
    std::atomic<bool> refreshRequested{false};
};