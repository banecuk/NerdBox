#pragma once

#include <ArduinoJson.h>

#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/JsonHttpService.h"
#include "utils/logging/LoggerInterface.h"

// Fetches and parses the AirVisual nearest-city API response.
// Call fetchData() from the background task — it writes results directly into
// the shared AirQualityData struct. fetchData() itself lives in JsonHttpService;
// the filter and the JSON -> AirQualityData mapping live in AirQualityParser
// (host-tested), so this class only adds the endpoint URL and the publish.
//
// Refresh cadence and widget staleness window: AppSettings'
// airQualityRefreshIntervalMs / airQualityStaleTimeoutMs.
class AirQualityService : public JsonHttpService<AirQualityData, AirQualityService> {
 public:
    // url must have static storage (AppSettings::endpoints, A9).
    AirQualityService(NetworkManager& networkManager, LoggerInterface& logger,
                      const AppSettings& config, const char* url);
    ~AirQualityService() = default;

 private:
    friend class JsonHttpService<AirQualityData, AirQualityService>;

    void initFilter(JsonDocument& filter);
    bool parseData(AirQualityData& outData);
};
