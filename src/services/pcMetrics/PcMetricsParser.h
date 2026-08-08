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

// One bit per top-level section under "Metrics".
enum Section : uint8_t {
    kCpu = 1 << 0,
    kCpuExtended = 1 << 1,
    kRam = 1 << 2,
    kGpu = 1 << 3,
    kMotherboard = 1 << 4,
    kDisks = 1 << 5,
    kNetwork = 1 << 6,
};

struct SectionResult {
    uint8_t sectionsSeen = 0;    // bitmask of Section: key present in the JSON
    uint8_t sectionsFailed = 0;  // bitmask of Section: present but parse returned false

    bool anySeen() const { return sectionsSeen != 0; }
    bool allSeenValid() const { return sectionsFailed == 0; }
    bool seen(Section s) const { return (sectionsSeen & s) != 0; }
};

// Walks every section of a "Metrics" object and dispatches each present one to
// its parse function above. Absent sections are skipped, never zeroed — the
// delta-mode contract. This function deliberately applies no publish rule: it
// only reports what was seen and what failed, so the full-report caller can
// demand "nothing failed" while the delta caller settles for "something was
// seen". Adding an 8th section is a one-line change here and nowhere else.
SectionResult parseAllSections(JsonObjectConst metrics, PcMetrics& outData,
                               uint8_t configuredCoreCount, LoggerInterface& logger);

}  // namespace PcMetricsParser
