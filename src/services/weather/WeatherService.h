#pragma once

#include <ArduinoJson.h>

#include "network/NetworkManager.h"
#include "services/JsonHttpService.h"
#include "services/weather/WeatherData.h"
#include "utils/logging/LoggerInterface.h"

// Fetches and parses the open-meteo daily forecast response.
// Call fetchData() from the background task — it writes results directly into
// the shared WeatherData struct. fetchData() itself lives in JsonHttpService;
// the filter and the JSON -> WeatherData mapping live in WeatherParser
// (host-tested), so this class only adds the endpoint URL and the publish.
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
};
