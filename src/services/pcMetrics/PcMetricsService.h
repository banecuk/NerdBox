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
    bool parseData(const String& rawData, PcMetrics& outData);
    void initFilter();
    bool validateJsonStructure(JsonObject metrics);
    bool parseCpuData(JsonObject cpu, PcMetrics& outData);
    bool parseRamData(JsonObject ram, PcMetrics& outData);
    bool parseGpuData(JsonObject gpu, PcMetrics& outData);
    bool parseMotherboardData(JsonObject motherboard, PcMetrics& outData);

    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;

    // Use heap allocation for JSON document to avoid stack overflow
    std::unique_ptr<JsonDocument> filterDoc_;
    JsonDocument filter_;  // Filter stays on stack (small size)
};