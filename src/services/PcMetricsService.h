#ifndef PCMETRICS_SERVICE_H
#define PCMETRICS_SERVICE_H

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/PcMetrics.h"
#include <ArduinoJson.h>

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

#endif