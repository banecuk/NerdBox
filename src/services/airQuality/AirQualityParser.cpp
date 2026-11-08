#include "AirQualityParser.h"

#include <cstring>

#include "services/JsonNumber.h"

namespace AirQualityParser {

void buildFilter(JsonDocument& filterDoc) {
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

bool parse(JsonObjectConst root, AirQualityData& outData, LoggerInterface& logger) {
    JsonObjectConst data = root["data"];
    if (data.isNull()) {
        logger.warning("AirQuality: missing 'data' key");
        return false;
    }

    JsonObjectConst weather = data["current"]["weather"];
    JsonObjectConst pollution = data["current"]["pollution"];
    if (weather.isNull() || pollution.isNull()) {
        logger.warning("AirQuality: missing weather/pollution keys");
        return false;
    }

    // Every numeric field goes through JsonNumber::roundAs: a fractional
    // node (e.g. "tp": 21.6) used to read as 0 via `| 0` (B5).
    outData.temperature = JsonNumber::roundAs<int8_t>(weather["tp"]);
    outData.humidity = JsonNumber::roundAs<uint8_t>(weather["hu"]);
    outData.pressure = JsonNumber::roundAs<int16_t>(weather["pr"]);
    outData.wind_dir = JsonNumber::roundAs<uint16_t>(weather["wd"]);
    // Wind speed arrives as a float (m/s); store x10 as an integer.
    outData.wind_speed_x10 = JsonNumber::roundAs<uint16_t>(weather["ws"], 10.0f);

    // Icon code — copy at most 3 chars ("01d", "10n", etc.)
    const char* ic = weather["ic"] | "";
    strncpy(outData.icon_code, ic, sizeof(outData.icon_code) - 1);
    outData.icon_code[sizeof(outData.icon_code) - 1] = '\0';

    outData.aqi_us = JsonNumber::roundAs<uint16_t>(pollution["aqius"]);
    return true;
}

}  // namespace AirQualityParser
