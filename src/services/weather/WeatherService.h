#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/weather/WeatherData.h"
#include "utils/LoggerInterface.h"

// Fetches and parses the open-meteo daily forecast response.
// Call fetchData() from the background task — it writes results directly into
// the shared WeatherData struct. A clone of AirQualityService.
//
// Refresh cadence: while the Weather screen is displayed (see WeatherJob);
// this class itself just does a single bounded fetch per call.
class WeatherService {
public:
    WeatherService(NetworkManager& networkManager, LoggerInterface& logger);
    ~WeatherService() = default;

    WeatherService(const WeatherService&)            = delete;
    WeatherService& operator=(const WeatherService&) = delete;

    // Fetches fresh data and writes it into outData.  Returns true on success.
    bool fetchData(WeatherData& outData);

    static constexpr unsigned long kRefreshIntervalMs =
        AppConfig::internal::WeatherImpl::kRefreshIntervalMs;

private:
    bool parseData(WeatherData& outData);
    void initFilter();

    NetworkManager&  networkManager_;
    LoggerInterface& logger_;

    // Reused across fetches to avoid heap fragmentation.
    std::unique_ptr<JsonDocument> doc_;
    JsonDocument filter_;  // Filter built once (small size)

    static constexpr uint8_t kMaxForecastDays = AppConfig::internal::WeatherImpl::kForecastDays;

    // Reads one daily array element as an x10-scaled integer, extracting the
    // float *before* the (rounded) cast to int — the FullscreenFps guard.
    static int16_t extractX10(const JsonArray& array, size_t index);
};