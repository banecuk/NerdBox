#include "WeatherService.h"

#include "utils/LogMacros.h"

WeatherService::WeatherService(NetworkManager& networkManager, LoggerInterface& logger)
    : JsonHttpService(networkManager, logger, "Weather", WEATHER_API) {
    init();
}

void WeatherService::initFilter(JsonDocument& filterDoc) {
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

int16_t WeatherService::extractX10(const JsonArray& array, size_t index) {
    if (array.isNull() || index >= array.size() || array[index].isNull()) {
        return 0;
    }
    // Extract as float first — an integer cast on the raw node would fail on a
    // float and silently return 0. Round half-away-from-zero so negative
    // values (below-freezing forecasts) round correctly too.
    const float scaled = array[index].as<float>() * 10.0f;
    return static_cast<int16_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

bool WeatherService::parseData(WeatherData& outData) {
    JsonObject daily = doc()["daily"];
    if (daily.isNull()) {
        logger_.warning("Weather: missing 'daily' key");
        return false;
    }

    JsonArray timeArray = daily["time"];
    if (timeArray.isNull() || timeArray.size() == 0) {
        logger_.warning("Weather: missing 'daily.time'");
        return false;
    }

    JsonArray weatherCode = daily["weather_code"];
    JsonArray tempMax = daily["temperature_2m_max"];
    JsonArray tempMin = daily["temperature_2m_min"];
    JsonArray rainSum = daily["rain_sum"];
    JsonArray windMax = daily["wind_speed_10m_max"];

    const uint8_t count = static_cast<uint8_t>(
        (timeArray.size() > kMaxForecastDays) ? kMaxForecastDays : timeArray.size());

    {
        ScopedLock lock(outData.daysMutex);
        for (uint8_t i = 0; i < count; ++i) {
            WeatherForecastDay& day = outData.days[i];
            day.weatherCode = static_cast<int16_t>(weatherCode[i] | 0);
            day.tempMaxX10 = extractX10(tempMax, i);
            day.tempMinX10 = extractX10(tempMin, i);
            day.rainX10 = extractX10(rainSum, i);
            day.windMaxX10 = extractX10(windMax, i);
            day.dayStart = static_cast<time_t>(timeArray[i].as<int64_t>());
        }
    }

    outData.dayCount = count;
    outData.freshness.publish(millis());
    outData.refreshRequested.store(false);

    LOG_DEBUGF(logger_,
               "Weather: %u days, day0 code=%d max=%d.%d°C min=%d.%d°C rain=%d.%dmm wind=%d.%dm/s",
               count, outData.days[0].weatherCode, outData.days[0].tempMaxX10 / 10,
               outData.days[0].tempMaxX10 % 10, outData.days[0].tempMinX10 / 10,
               outData.days[0].tempMinX10 % 10, outData.days[0].rainX10 / 10,
               outData.days[0].rainX10 % 10, outData.days[0].windMaxX10 / 10,
               outData.days[0].windMaxX10 % 10);

    return true;
}
