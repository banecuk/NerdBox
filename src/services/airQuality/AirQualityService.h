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
// Refresh cadence: 30 minutes — within the AirVisual free-tier rate limit.
class AirQualityService : public JsonHttpService<AirQualityData, AirQualityService> {
 public:
    AirQualityService(NetworkManager& networkManager, LoggerInterface& logger);
    ~AirQualityService() = default;

    static constexpr unsigned long kRefreshIntervalMs = 30UL * 60UL * 1000UL;

    // Widget staleness window. AirQualityJob only fetches once the data is
    // already kRefreshIntervalMs old, so using the interval itself blanked
    // the widget on every refresh (B42) — same 3x rule as RoomClimateService.
    static constexpr unsigned long kStaleTimeoutMs = 3UL * kRefreshIntervalMs;

 private:
    friend class JsonHttpService<AirQualityData, AirQualityService>;

    void initFilter(JsonDocument& filter);
    bool parseData(AirQualityData& outData);
};
