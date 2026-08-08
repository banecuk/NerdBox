#include "AirQualityService.h"

AirQualityService::AirQualityService(NetworkManager& networkManager, LoggerInterface& logger)
    : JsonHttpService(networkManager, logger, "AirQuality", AIR_VISUAL_API) {
    init();
}

void AirQualityService::initFilter(JsonDocument& filterDoc) {
    JsonObject filter = filterDoc.to<JsonObject>();
    JsonObject weather = filter["data"]["current"]["weather"].to<JsonObject>();
    weather["tp"] = true;
    weather["hu"] = true;
    weather["pr"] = true;
    weather["ws"] = true;
    weather["wd"] = true;
    weather["ic"] = true;

    JsonObject pollution = filter["data"]["current"]["pollution"].to<JsonObject>();
    pollution["aqius"] = true;
}

bool AirQualityService::parseData(AirQualityData& outData) {
    JsonObject data = doc()["data"];
    if (data.isNull()) {
        logger_.warning("AirQuality: missing 'data' key");
        return false;
    }

    JsonObject weather = data["current"]["weather"];
    JsonObject pollution = data["current"]["pollution"];

    if (weather.isNull() || pollution.isNull()) {
        logger_.warning("AirQuality: missing weather/pollution keys");
        return false;
    }

    outData.temperature = static_cast<int8_t>(weather["tp"] | 0);
    outData.humidity = static_cast<uint8_t>(weather["hu"] | 0);
    outData.pressure = static_cast<int16_t>(weather["pr"] | 0);
    outData.wind_dir = static_cast<uint16_t>(weather["wd"] | 0);

    // Icon code — copy at most 3 chars ("01d", "10n", etc.)
    const char* ic = weather["ic"] | "";
    strncpy(outData.icon_code, ic, sizeof(outData.icon_code) - 1);
    outData.icon_code[sizeof(outData.icon_code) - 1] = '\0';

    // Wind speed arrives as a float (m/s); store x10 as an integer.
    const float ws = weather["ws"].isNull() ? 0.0f : weather["ws"].as<float>();
    outData.wind_speed_x10 = static_cast<uint16_t>(ws * 10.0f + 0.5f);

    outData.aqi_us = static_cast<uint16_t>(pollution["aqius"] | 0);
    outData.freshness.publish(millis());

    logger_.debugf("AirQuality: tp=%d hu=%d pr=%d ws=%.1f wd=%d aqi=%d", outData.temperature,
                   outData.humidity, outData.pressure, ws, outData.wind_dir, outData.aqi_us);

    return true;
}
