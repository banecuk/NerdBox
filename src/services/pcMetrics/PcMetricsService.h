#pragma once

#include <ArduinoJson.h>

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
    bool fetchData(PcMetrics& outData);

 private:
    struct HardwareIndices {
        int motherboard;
        int cpu;
        int memory;
        int gpu;
    };

    void initFilter();
    bool parseData(const String& rawData, PcMetrics& outData);

    HardwareIndices findHardwareIndices(JsonArray hardwareChildren);
    bool parseMotherboard(JsonArray hardwareChildren, int index, PcMetrics& outData);
    bool parseCpu(JsonArray hardwareChildren, int index, PcMetrics& outData);
    bool parseMemory(JsonArray hardwareChildren, int index, PcMetrics& outData);
    bool parseGpu(JsonArray hardwareChildren, int index, PcMetrics& outData);

    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;

    JsonDocument filter_;
};