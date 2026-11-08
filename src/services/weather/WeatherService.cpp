#include "WeatherService.h"

#include "config/Environment.h"
#include "services/weather/WeatherParser.h"
#include "utils/logging/LogMacros.h"

WeatherService::WeatherService(NetworkManager& networkManager, LoggerInterface& logger)
    : JsonHttpService(networkManager, logger, "Weather", WEATHER_API) {
    init();
}

void WeatherService::initFilter(JsonDocument& filterDoc) {
    WeatherParser::buildFilter(filterDoc);
}

bool WeatherService::parseData(WeatherData& outData) {
    if (!WeatherParser::parse(doc().as<JsonObjectConst>(), outData, logger_)) {
        return false;
    }

    outData.freshness.publish(millis());
    // refreshRequested is consumed by WeatherJob when the fetch starts, not
    // here — clearing it after parsing wiped any request raised while this
    // fetch was in flight (e.g. a midnight rollover, B60).

    LOG_DEBUGF(logger_,
               "Weather: %u days, day0 code=%d max=%d.%d°C min=%d.%d°C rain=%d.%dmm wind=%d.%dm/s",
               outData.dayCount, outData.days[0].weatherCode, outData.days[0].tempMaxX10 / 10,
               outData.days[0].tempMaxX10 % 10, outData.days[0].tempMinX10 / 10,
               outData.days[0].tempMinX10 % 10, outData.days[0].rainX10 / 10,
               outData.days[0].rainX10 % 10, outData.days[0].windMaxX10 / 10,
               outData.days[0].windMaxX10 % 10);

    return true;
}
