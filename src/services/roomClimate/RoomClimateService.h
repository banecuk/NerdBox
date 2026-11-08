#pragma once

#include <ArduinoJson.h>

#include "network/NetworkManager.h"
#include "services/JsonHttpService.h"
#include "services/roomClimate/RoomClimateData.h"
#include "services/roomClimate/RoomClimateMath.h"
#include "utils/logging/LoggerInterface.h"

// Fetches and parses the local room sensor's tiny JSON response
// ({"temperature":26.55,"humidity":45.08}, no wrapper object).
// Call fetchData() from the background task — it writes results directly into
// the shared RoomClimateData struct. fetchData() itself lives in
// JsonHttpService; only the filter and the parse are specific to this
// endpoint.
//
// Refresh cadence and widget staleness window: AppSettings'
// roomClimateRefreshIntervalMs / roomClimateStaleTimeoutMs.
class RoomClimateService : public JsonHttpService<RoomClimateData, RoomClimateService> {
 public:
    // url must have static storage (AppSettings::endpoints, A9).
    RoomClimateService(NetworkManager& networkManager, LoggerInterface& logger,
                       const AppSettings& config, const char* url);
    ~RoomClimateService() = default;

 private:
    friend class JsonHttpService<RoomClimateData, RoomClimateService>;

    void initFilter(JsonDocument& filter);
    bool parseData(RoomClimateData& outData);
};
