#pragma once

#include <Arduino.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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

// RAII guard: locks daysMutex on construction, releases on destruction.
// Usage:
//   { WeatherDataLock lock(weatherData); use weatherData.days; }
class WeatherData;
class WeatherDataLock {
 public:
    explicit WeatherDataLock(WeatherData& d);
    ~WeatherDataLock();
    WeatherDataLock(const WeatherDataLock&) = delete;
    WeatherDataLock& operator=(const WeatherDataLock&) = delete;

 private:
    WeatherData& d_;
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

    bool is_available = false;
    unsigned long last_update = 0;   // millis() timestamp of last successful fetch

    // Mutex protecting days[] only — the background fetch (background task)
    // and WeatherWidget's draw (screen task) both touch it. is_available /
    // last_update / dayCount / refreshRequested are word-sized and accessed
    // as naturally atomic, mirroring PcMetrics's disk_drives lock pattern.
    SemaphoreHandle_t daysMutex = xSemaphoreCreateMutex();

    WeatherForecastDay days[AppConfig::internal::WeatherImpl::kForecastDays];
    uint8_t dayCount = 0;

    // Written by the screen task and read by the background job — signals a
    // local-midnight rollover so the forecast refetches and shows the new day.
    std::atomic<bool> refreshRequested{false};
};

// Inline RAII implementation — defined here so every TU that includes
// WeatherData.h can use WeatherDataLock without a separate .cpp.
inline WeatherDataLock::WeatherDataLock(WeatherData& d) : d_(d) {
    xSemaphoreTake(d_.daysMutex, portMAX_DELAY);
}

inline WeatherDataLock::~WeatherDataLock() {
    xSemaphoreGive(d_.daysMutex);
}