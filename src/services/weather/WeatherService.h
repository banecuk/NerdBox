#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "config/Limits.h"
#include "network/NetworkManager.h"
#include "services/JsonHttpService.h"
#include "services/weather/WeatherData.h"
#include "utils/LoggerInterface.h"
#include "utils/ScopedLock.h"

// Fetches and parses the open-meteo daily forecast response.
// Call fetchData() from the background task — it writes results directly into
// the shared WeatherData struct. fetchData() itself lives in JsonHttpService;
// only the filter and the parse are specific to this endpoint.
//
// Refresh cadence: every ~2h regardless of active screen, plus on-demand on
// Weather screen entry (see WeatherJob); this class itself just does a
// single bounded fetch per call.
class WeatherService : public JsonHttpService<WeatherData, WeatherService> {
 public:
    WeatherService(NetworkManager& networkManager, LoggerInterface& logger);
    ~WeatherService() = default;

 private:
    friend class JsonHttpService<WeatherData, WeatherService>;

    void initFilter(JsonDocument& filter);
    bool parseData(WeatherData& outData);

    static constexpr uint8_t kMaxForecastDays = AppConfig::Limits::kForecastDays;

    // Reads one daily array element as an x10-scaled integer, extracting the
    // float *before* the (rounded) cast to int — the FullscreenFps guard.
    static int16_t extractX10(const JsonArray& array, size_t index);
};
