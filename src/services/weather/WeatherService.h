#pragma once

#include <ArduinoJson.h>

#include "config/AppConfig.h"
#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/JsonHttpService.h"
#include "services/weather/WeatherData.h"
#include "utils/LoggerInterface.h"

// Fetches and parses the open-meteo daily forecast response.
// Call fetchData() from the background task — it writes results directly into
// the shared WeatherData struct. fetchData() itself lives in JsonHttpService;
// only the filter and the parse are specific to this endpoint.
//
// Refresh cadence: while the Weather screen is displayed (see WeatherJob);
// this class itself just does a single bounded fetch per call.
class WeatherService : public JsonHttpService<WeatherData, WeatherService> {
public:
    WeatherService(NetworkManager& networkManager, LoggerInterface& logger);
    ~WeatherService() = default;

    static constexpr unsigned long kRefreshIntervalMs =
        AppConfig::internal::WeatherImpl::kRefreshIntervalMs;

private:
    friend class JsonHttpService<WeatherData, WeatherService>;

    void initFilter(JsonDocument& filter);
    bool parseData(WeatherData& outData);

    static constexpr uint8_t kMaxForecastDays = AppConfig::internal::WeatherImpl::kForecastDays;

    // Reads one daily array element as an x10-scaled integer, extracting the
    // float *before* the (rounded) cast to int — the FullscreenFps guard.
    static int16_t extractX10(const JsonArray& array, size_t index);
};
