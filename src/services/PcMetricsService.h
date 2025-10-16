#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/PcMetrics.h"
#include "utils/ApplicationMetrics.h"
#include "utils/LoggerInterface.h"
#include "config/AppConfigInterface.h"

class PcMetricsService {
   public:
    PcMetricsService(NetworkManager &networkManager, ApplicationMetrics &systemMetrics, LoggerInterface& logger, AppConfigInterface& config);
    bool fetchData(PcMetrics &outData);

   private:
    void initFilter();
    bool parseData(const String &rawData, PcMetrics &outData);

    NetworkManager &networkManager_;
    ApplicationMetrics &systemMetrics_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;

    JsonDocument filter_;
};