#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "config/AppSettings.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/SseEventParser.h"
#include "utils/logging/LoggerInterface.h"

// The streaming counterpart to PcMetricsService: owns the JsonDocument, the
// shared PcMetricsParser filter, and the delta-mode publish rule for one SSE
// event's `data:` payload. Extracted from PcMetricsStreamJob so that rule —
// "any section present is enough to count as fresh," which differs from the
// polling path's "no present section failed" — is host-testable instead of
// only being enforced by a comment. PcMetricsStreamJob keeps connect/backoff/
// staleness (scheduling policy); this class keeps parse/filter/publish (data
// logic), mirroring the PcMetricsJob/PcMetricsService split.
class PcMetricsStreamService {
 public:
    PcMetricsStreamService(PcMetrics& metrics, const AppSettings& config, LoggerInterface& logger);

    // Deserializes one SSE event's `data:` payload and, if it carries any
    // recognized section, applies it to the shared PcMetrics instance and
    // publishes a fresh timestamp.
    void handleEvent(const SseEventParser::Event& event);

 private:
    PcMetrics& metrics_;
    const AppSettings& config_;
    LoggerInterface& logger_;

    std::unique_ptr<JsonDocument> doc_;  // reused across events, like PcMetricsService::doc_
    JsonDocument filter_;
};
