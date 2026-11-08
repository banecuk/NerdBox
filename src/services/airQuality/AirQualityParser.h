#pragma once

#include <ArduinoJson.h>

#include "services/airQuality/AirQualityData.h"
#include "utils/logging/LoggerInterface.h"

// "AirVisual nearest_city JSON -> AirQualityData" as free functions, following
// the PcMetricsParser pattern. No network or Environment.h dependency, so the
// mapping is host-tested under [env:native] (test/AirQualityParserTest.cpp);
// AirQualityService keeps only the fetch.
namespace AirQualityParser {

// The ArduinoJson filter for exactly the fields parse() reads.
void buildFilter(JsonDocument& filter);

// Fills outData's weather + pollution fields from the response root. Returns
// false, leaving outData untouched, when data.current.weather or
// data.current.pollution is missing. Does not publish freshness — that's the
// caller's job, since it needs the clock.
bool parse(JsonObjectConst root, AirQualityData& outData, LoggerInterface& logger);

}  // namespace AirQualityParser
