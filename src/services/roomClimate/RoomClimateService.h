#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
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
// Refresh cadence: 60s — it's a LAN device, so the fetch is cheap.
class RoomClimateService : public JsonHttpService<RoomClimateData, RoomClimateService> {
 public:
    RoomClimateService(NetworkManager& networkManager, LoggerInterface& logger);
    ~RoomClimateService() = default;

    static constexpr unsigned long kRefreshIntervalMs = 60UL * 1000UL;

    // Widget staleness window: a single missed 60s fetch shouldn't flip the
    // display to "No Data" for a full minute, which reads as a fault rather
    // than a blip.
    static constexpr unsigned long kStaleTimeoutMs = 3UL * kRefreshIntervalMs;

 private:
    friend class JsonHttpService<RoomClimateData, RoomClimateService>;

    void initFilter(JsonDocument& filter);
    bool parseData(RoomClimateData& outData);
};
