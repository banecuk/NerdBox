#include "RoomClimateService.h"

#include "utils/logging/LogMacros.h"

RoomClimateService::RoomClimateService(NetworkManager& networkManager, LoggerInterface& logger)
    : JsonHttpService(networkManager, logger, "RoomClimate", ROOM_CLIMATE_API) {
    init();
}

void RoomClimateService::initFilter(JsonDocument& filterDoc) {
    JsonObject filter = filterDoc.to<JsonObject>();
    filter["temperature"] = true;
    filter["humidity"] = true;
}

bool RoomClimateService::parseData(RoomClimateData& outData) {
    JsonVariant temperature = doc()["temperature"];
    JsonVariant humidity = doc()["humidity"];

    if (temperature.isNull() || humidity.isNull()) {
        logger_.warning("RoomClimate: missing temperature/humidity key");
        return false;
    }

    outData.temperature_x10 = RoomClimateMath::toX10(temperature.as<float>());
    outData.humidity = RoomClimateMath::clampHumidity(humidity.as<float>());

    outData.freshness.publish(millis());

    LOG_DEBUGF(logger_, "RoomClimate: temp=%d.%d hu=%d", outData.temperature_x10 / 10,
               outData.temperature_x10 % 10, outData.humidity);

    return true;
}
