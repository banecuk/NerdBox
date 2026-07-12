#pragma once
#include <ArduinoJson.h>

#include <memory>

#include "config/AppSettings.h"
#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "utils/ApplicationMetrics.h"
#include "utils/LoggerInterface.h"

class PcMetricsService {
 public:
    PcMetricsService(NetworkManager& networkManager, ApplicationMetrics& systemMetrics,
                     LoggerInterface& logger, const AppSettings& config);
    ~PcMetricsService() = default;

    // Delete copy operations
    PcMetricsService(const PcMetricsService&) = delete;
    PcMetricsService& operator=(const PcMetricsService&) = delete;

    bool fetchData(PcMetrics& outData);

    // Debug counters — surfaced over /api/status to distinguish "PC app down"
    // from "parse broken" from "WiFi flaky".
    uint32_t getFetchOkCount() const { return fetchOk_; }
    uint32_t getFetchFailCount() const { return fetchFail_; }
    const char* getLastError() const { return lastError_; }

    // Dedicated, unfiltered fetch of the raw NerdWinSense payload for
    // GET /api/raw. Uses its own short-lived HTTPClient rather than
    // networkManager_.getHttpClient() — that instance is shared with the
    // background task's regular polling fetchData() and is not safe to use
    // concurrently from the web server's task on the other core. Not part of
    // the regular polling path, so it costs nothing during normal operation.
    bool fetchRawJson(String& outRaw);

 private:
    bool parseData(PcMetrics& outData);
    void initFilter();
    bool parseCpuData(JsonObject cpu, PcMetrics& outData);
    bool parseCpuExtendedData(JsonObject cpuExtended, PcMetrics& outData);
    bool parseRamData(JsonObject ram, PcMetrics& outData);
    bool parseGpuData(JsonObject gpu, PcMetrics& outData);
    bool parseMotherboardData(JsonObject motherboard, PcMetrics& outData);
    bool parseDiskData(JsonObject disks, PcMetrics& outData);
    bool parseNetworkData(JsonObject network, PcMetrics& outData);

    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
    LoggerInterface& logger_;
    const AppSettings& config_;

    // Use heap allocation for JSON document to avoid stack overflow
    std::unique_ptr<JsonDocument> doc_;  // Reused across fetches to avoid heap fragmentation
    JsonDocument filter_;                // Filter stays on stack (small size)

    uint32_t fetchOk_ = 0;
    uint32_t fetchFail_ = 0;
    char lastError_[64] = "";
};