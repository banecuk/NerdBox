#include "WeatherParser.h"

#include "config/Limits.h"
#include "services/JsonNumber.h"
#include "utils/ScopedLock.h"

namespace WeatherParser {

void buildFilter(JsonDocument& filterDoc) {
    JsonObject filter = filterDoc.to<JsonObject>();
    JsonObject daily = filter["daily"].to<JsonObject>();
    daily["time"] = true;
    daily["weather_code"] = true;
    daily["temperature_2m_max"] = true;
    daily["temperature_2m_min"] = true;
    daily["rain_sum"] = true;
    daily["wind_speed_10m_max"] = true;
    filter["daily_units"] = true;
}

bool parse(JsonObjectConst root, WeatherData& outData, LoggerInterface& logger) {
    JsonObjectConst daily = root["daily"];
    if (daily.isNull()) {
        logger.warning("Weather: missing 'daily' key");
        return false;
    }

    JsonArrayConst timeArray = daily["time"];
    if (timeArray.isNull() || timeArray.size() == 0) {
        logger.warning("Weather: missing 'daily.time'");
        return false;
    }

    JsonArrayConst weatherCode = daily["weather_code"];
    JsonArrayConst tempMax = daily["temperature_2m_max"];
    JsonArrayConst tempMin = daily["temperature_2m_min"];
    JsonArrayConst rainSum = daily["rain_sum"];
    JsonArrayConst windMax = daily["wind_speed_10m_max"];

    constexpr size_t kMaxDays = AppConfig::Limits::kForecastDays;
    const uint8_t count =
        static_cast<uint8_t>(timeArray.size() > kMaxDays ? kMaxDays : timeArray.size());

    {
        ScopedLock lock(outData.daysMutex);
        for (uint8_t i = 0; i < count; ++i) {
            WeatherForecastDay& day = outData.days[i];
            // A missing array or index reads as a null node -> 0.
            day.weatherCode = JsonNumber::roundAs<int16_t>(weatherCode[i]);
            day.tempMaxX10 = JsonNumber::roundAs<int16_t>(tempMax[i], 10.0f);
            day.tempMinX10 = JsonNumber::roundAs<int16_t>(tempMin[i], 10.0f);
            day.rainX10 = JsonNumber::roundAs<int16_t>(rainSum[i], 10.0f);
            day.windMaxX10 = JsonNumber::roundAs<int16_t>(windMax[i], 10.0f);
            day.dayStart = static_cast<time_t>(timeArray[i].as<int64_t>());
        }
    }

    outData.dayCount = count;
    return true;
}

}  // namespace WeatherParser
