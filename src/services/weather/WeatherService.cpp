#include "WeatherService.h"

#include "config/AppConfig.h"

WeatherService::WeatherService(NetworkManager& networkManager, LoggerInterface& logger)
    : networkManager_(networkManager),
      logger_(logger),
      doc_(std::make_unique<JsonDocument>()) {
    initFilter();
}

void WeatherService::initFilter() {
    JsonObject filter = filter_.to<JsonObject>();
    JsonObject daily  = filter["daily"].to<JsonObject>();
    daily["time"] = true;
    daily["weather_code"] = true;
    daily["temperature_2m_max"] = true;
    daily["temperature_2m_min"] = true;
    daily["rain_sum"] = true;
    daily["wind_speed_10m_max"] = true;
    filter["daily_units"] = true;
}

bool WeatherService::fetchData(WeatherData& outData) {
    if (!networkManager_.isConnected()) {
        logger_.warning("Weather: network not connected");
        return false;
    }

    HttpClient& http = networkManager_.getHttpClient();
    doc_->clear();
    // open-meteo is an internet-facing API — give it the same headroom as
    // AirVisual so a legitimately slow response isn't treated as a failure.
    if (!http.downloadAndParse(WEATHER_API, *doc_, filter_, 2, 100, 3000, 6000)) {
        if (http.getLastHttpCode() == HTTP_CODE_OK) {
            logger_.warningf("Weather: JSON parse error: %s", http.getLastParseError().c_str());
        } else {
            logger_.errorf("Weather: HTTP GET failed, code: %d: %s", http.getLastHttpCode(),
                           http.getLastErrorBody().c_str());
        }
        return false;
    }

    return parseData(outData);
}

int16_t WeatherService::extractX10(const JsonArray& array, size_t index) {
    if (array.isNull() || index >= array.size() || array[index].isNull()) {
        return 0;
    }
    // Extract as float first — an integer cast on the raw node would fail on a
    // float and silently return 0.
    return static_cast<int16_t>(array[index].as<float>() * 10.0f + 0.5f);
}

bool WeatherService::parseData(WeatherData& outData) {
    JsonObject daily = (*doc_)["daily"];
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
    JsonArray tempMax     = daily["temperature_2m_max"];
    JsonArray tempMin     = daily["temperature_2m_min"];
    JsonArray rainSum     = daily["rain_sum"];
    JsonArray windMax     = daily["wind_speed_10m_max"];

    const uint8_t count = static_cast<uint8_t>(
        (timeArray.size() > kMaxForecastDays) ? kMaxForecastDays : timeArray.size());

    for (uint8_t i = 0; i < count; ++i) {
        WeatherForecastDay& day = outData.days[i];
        day.weatherCode         = static_cast<int16_t>(weatherCode[i] | 0);
        day.tempMaxX10          = extractX10(tempMax, i);
        day.tempMinX10          = extractX10(tempMin, i);
        day.rainX10             = extractX10(rainSum, i);
        day.windMaxX10          = extractX10(windMax, i);
        day.dayStart            = static_cast<time_t>(timeArray[i].as<int64_t>());
    }

    outData.dayCount     = count;
    outData.is_available = true;
    outData.last_update  = millis();
    outData.refreshRequested.store(false);

    logger_.debugf("Weather: %u days, day0 code=%d max=%d.%d°C min=%d.%d°C rain=%d.%dmm wind=%d.%dm/s",
                   count, outData.days[0].weatherCode,
                   outData.days[0].tempMaxX10 / 10, outData.days[0].tempMaxX10 % 10,
                   outData.days[0].tempMinX10 / 10, outData.days[0].tempMinX10 % 10,
                   outData.days[0].rainX10 / 10, outData.days[0].rainX10 % 10,
                   outData.days[0].windMaxX10 / 10, outData.days[0].windMaxX10 % 10);

    return true;
}
