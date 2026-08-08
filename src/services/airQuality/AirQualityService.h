#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "services/JsonHttpService.h"
#include "utils/LoggerInterface.h"

// Fetches and parses the AirVisual nearest-city API response.
// Call fetchData() from the background task — it writes results directly into
// the shared AirQualityData struct. fetchData() itself lives in JsonHttpService;
// only the filter and the parse are specific to this endpoint.
//
// Refresh cadence: 30 minutes — within the AirVisual free-tier rate limit.
class AirQualityService : public JsonHttpService<AirQualityData, AirQualityService> {
 public:
    AirQualityService(NetworkManager& networkManager, LoggerInterface& logger);
    ~AirQualityService() = default;

    static constexpr unsigned long kRefreshIntervalMs = 30UL * 60UL * 1000UL;

 private:
    friend class JsonHttpService<AirQualityData, AirQualityService>;

    void initFilter(JsonDocument& filter);
    bool parseData(AirQualityData& outData);
};
