#include "AirQualityService.h"

#include "services/airQuality/AirQualityParser.h"
#include "utils/logging/LogMacros.h"

AirQualityService::AirQualityService(NetworkManager& networkManager, LoggerInterface& logger,
                                     const AppSettings& config, const char* url)
    : JsonHttpService(networkManager, logger, config, "AirQuality", url) {
    init();
}

void AirQualityService::initFilter(JsonDocument& filterDoc) {
    AirQualityParser::buildFilter(filterDoc);
}

bool AirQualityService::parseData(AirQualityData& outData) {
    if (!AirQualityParser::parse(doc().as<JsonObjectConst>(), outData, logger_)) {
        return false;
    }
    outData.freshness.publish(millis());

    LOG_DEBUGF(logger_, "AirQuality: tp=%d hu=%d pr=%d ws=%u.%u wd=%d aqi=%d", outData.temperature,
               outData.humidity, outData.pressure, outData.wind_speed_x10 / 10,
               outData.wind_speed_x10 % 10, outData.wind_dir, outData.aqi_us);

    return true;
}
