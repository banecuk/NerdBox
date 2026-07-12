#pragma once
#include <ArduinoJson.h>

#include "services/pcMetrics/PcMetrics.h"
#include "utils/LoggerInterface.h"

// "NerdWinSense JSON section -> PcMetrics fields" — the single source of
// truth for this mapping, shared by the polling path (PcMetricsService,
// always a full report) and the future SSE streaming path (delta mode,
// where a section or field can be legitimately absent).
//
// Delta-mode contract: a key that is not present in the JSON object means
// "unchanged since the last update", not "zero this field out". Every
// function here must only touch an outData field when its JSON key is
// actually present — never fall back to a default value for a missing key.
namespace PcMetricsParser {

// Builds the ArduinoJson filter describing exactly the fields the parsers
// below read — shared by the polling fetch (PcMetricsService) and the SSE
// stream (PcMetricsStreamJob) so there is exactly one definition of "which
// NerdWinSense fields this firmware cares about".
void buildFilter(JsonDocument& filter);

bool parseCpuData(JsonObjectConst cpu, PcMetrics& outData, uint8_t configuredCoreCount,
                   LoggerInterface& logger);
bool parseCpuExtendedData(JsonObjectConst cpuExtended, PcMetrics& outData);
bool parseRamData(JsonObjectConst ram, PcMetrics& outData);
bool parseGpuData(JsonObjectConst gpu, PcMetrics& outData);
bool parseMotherboardData(JsonObjectConst motherboard, PcMetrics& outData);
bool parseDiskData(JsonObjectConst disks, PcMetrics& outData);
bool parseNetworkData(JsonObjectConst network, PcMetrics& outData);

}  // namespace PcMetricsParser
