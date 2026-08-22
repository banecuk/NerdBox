#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "services/pcMetrics/SseEventParser.h"
#include "services/processes/ProcessData.h"
#include "utils/logging/LoggerInterface.h"

// Parse/publish half of the process-list SSE stream, mirroring
// CpuClockStreamService's split from its job. Every event is a full
// `{"TopByCpu":[...],"TopByRam":[...],"TopByDisk":[...]}` snapshot (no delta
// mode, no wrapper) — a missing list is simply left untouched (same
// defensive habit as the other parsers, costs nothing).
class ProcessStreamService {
 public:
    ProcessStreamService(ProcessData& data, LoggerInterface& logger);

    // Deserializes one SSE event's `data:` payload and, if it carries any of
    // the three recognized lists, applies it to the shared ProcessData
    // instance and publishes a fresh timestamp.
    void handleEvent(const SseEventParser::Event& event);

 private:
    static uint8_t parseList(JsonArrayConst arr,
                             ProcessEntry (&out)[ProcessData::kEntriesPerList]);

    ProcessData& data_;
    LoggerInterface& logger_;

    std::unique_ptr<JsonDocument> doc_;  // reused across events
    JsonDocument filter_;
};
