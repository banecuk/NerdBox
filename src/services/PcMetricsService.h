#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/PcMetrics.h"
#include "utils/ApplicationMetrics.h"

class PcMetricsService {
   public:
    PcMetricsService(NetworkManager &networkManager, ApplicationMetrics &systemMetrics);
    bool fetchData(PcMetrics &outData);

   private:
    void initFilter();
    bool parseData(const String &rawData, PcMetrics &outData);

    NetworkManager &networkManager_;
    ApplicationMetrics &systemMetrics_;
    JsonDocument filter_;
};