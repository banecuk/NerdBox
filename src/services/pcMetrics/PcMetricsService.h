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

 private:
    bool parseData(PcMetrics& outData);
    void initFilter();
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
    JsonDocument filter_;                // Filter stays on stack (small size)
    bool filterInitialized_ = false;
};