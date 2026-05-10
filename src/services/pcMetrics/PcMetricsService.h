#pragma once
#include <ArduinoJson.h>

#include <memory>

#include "config/AppConfigInterface.h"
#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/pcMetrics/PcMetrics.h"
#include "utils/ApplicationMetrics.h"
#include "utils/LoggerInterface.h"

class PcMetricsService {
 public:
    PcMetricsService(NetworkManager& networkManager, ApplicationMetrics& systemMetrics,
                     LoggerInterface& logger, AppConfigInterface& config);
    ~PcMetricsService() = default;

    // Delete copy operations
    PcMetricsService(const PcMetricsService&) = delete;
    PcMetricsService& operator=(const PcMetricsService&) = delete;

    bool fetchData(PcMetrics& outData);

    bool isDataStale() const {
        return (millis() - lastSuccessfulFetchTime_ > DATA_STALE_TIMEOUT_MS);
    }

    // New method to get time since last successful fetch
    unsigned long getTimeSinceLastUpdate() const { return millis() - lastSuccessfulFetchTime_; }

 private:
    bool parseData(const String& rawData, PcMetrics& outData);
    void initFilter();
    bool validateJsonStructure(JsonObject metrics);
    bool parseCpuData(JsonObject cpu, PcMetrics& outData);
    bool parseCpuExtendedData(JsonObject cpuExtended, PcMetrics& outData);
    bool parseRamData(JsonObject ram, PcMetrics& outData);
    bool parseGpuData(JsonObject gpu, PcMetrics& outData);
    bool parseMotherboardData(JsonObject motherboard, PcMetrics& outData);
    bool parseDiskData(JsonObject disks, PcMetrics& outData);

    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;

    // Use heap allocation for JSON document to avoid stack overflow
    std::unique_ptr<JsonDocument> filterDoc_;
    std::unique_ptr<JsonDocument> doc_;  // Reused across fetches to avoid heap fragmentation
    JsonDocument filter_;  // Filter stays on stack (small size)
    bool filterInitialized_ = false;

    String rawData_;  // Reused across fetches — avoids a heap alloc + free every 300–500 ms

    // New members for tracking data freshness
    unsigned long lastSuccessfulFetchTime_ = 0;
    static constexpr unsigned long DATA_STALE_TIMEOUT_MS = 5000;  // 5 seconds
};