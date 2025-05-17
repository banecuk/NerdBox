#pragma once

#include <ArduinoJson.h>

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/PcMetrics.h"

class PcMetricsService {
   public:
    PcMetricsService(NetworkManager &networkManager);
    bool fetchData(PcMetrics &outData);

   private:
    void initFilter();
    bool parseData(const String &rawData, PcMetrics &outData);

    NetworkManager &networkManager_;
    JsonDocument filter_;
};