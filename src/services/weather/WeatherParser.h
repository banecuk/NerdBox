#pragma once

#include <ArduinoJson.h>

#include "services/weather/WeatherData.h"
#include "utils/logging/LoggerInterface.h"

// "open-meteo daily forecast JSON -> WeatherData" as free functions, following
// the PcMetricsParser pattern. No network or Environment.h dependency, so the
// mapping is host-tested under [env:native] (test/WeatherParserTest.cpp);
// WeatherService keeps only the fetch.
namespace WeatherParser {

// The ArduinoJson filter for exactly the fields parse() reads.
void buildFilter(JsonDocument& filter);

// Fills outData.days[] (under daysMutex) and outData.dayCount from the
// response root. Returns false, leaving outData untouched, when the `daily`
// object or `daily.time` is missing. Does not publish freshness — that's the
// caller's job, since it needs the clock.
bool parse(JsonObjectConst root, WeatherData& outData, LoggerInterface& logger);

}  // namespace WeatherParser
