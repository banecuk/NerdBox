#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/airQuality/AirQualityData.h"
#include "utils/LoggerInterface.h"

// Fetches and parses the AirVisual nearest-city API response.
// Call fetchData() from the background task — it writes results directly into
// the shared AirQualityData struct.
//
// Refresh cadence: 30 minutes — within the AirVisual free-tier rate limit.
class AirQualityService {
public:
    AirQualityService(NetworkManager& networkManager, LoggerInterface& logger);
    ~AirQualityService() = default;

    AirQualityService(const AirQualityService&)            = delete;
    AirQualityService& operator=(const AirQualityService&) = delete;

    // Fetches fresh data and writes it into outData.  Returns true on success.
    bool fetchData(AirQualityData& outData);

    static constexpr unsigned long kRefreshIntervalMs = 30UL * 60UL * 1000UL;

private:
    bool parseData(AirQualityData& outData);
    void initFilter();

    NetworkManager&  networkManager_;
    LoggerInterface& logger_;

    // Reused across fetches to avoid heap fragmentation.
    std::unique_ptr<JsonDocument> doc_;
    JsonDocument filter_;  // Filter built once (small size)
};
