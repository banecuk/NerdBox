#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "services/cpuClock/CpuClockData.h"
#include "services/pcMetrics/SseEventParser.h"
#include "utils/logging/LoggerInterface.h"

// Parse/publish half of the CPU-clock SSE stream, mirroring
// PcMetricsStreamService's split from its job. Unlike the main PC-metrics
// stream, this endpoint has no delta mode and no `Metrics` wrapper — every
// event is a full `{"CoreClocksMHz":[...],"BusSpeedMHz":...}` snapshot, so
// there is no delta-merge rule to enforce; a missing key is simply left
// untouched (same defensive habit as the other parsers, costs nothing).
class CpuClockStreamService {
 public:
    CpuClockStreamService(CpuClockData& data, LoggerInterface& logger);

    // Deserializes one SSE event's `data:` payload and, if it carries either
    // recognized field, applies it to the shared CpuClockData instance and
    // publishes a fresh timestamp.
    void handleEvent(const SseEventParser::Event& event);

 private:
    CpuClockData& data_;
    LoggerInterface& logger_;

    std::unique_ptr<JsonDocument> doc_;  // reused across events
    JsonDocument filter_;
};
