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
    bool parseData(const String& rawData, PcMetrics& outData);
    void initFilter();

    NetworkManager& networkManager_;
    ApplicationMetrics& systemMetrics_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;
    JsonDocument filter_;  // JSON filter for efficient parsing
};