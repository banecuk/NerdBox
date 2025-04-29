#ifndef PCMETRICS_SERVICE_H
#define PCMETRICS_SERVICE_H

#include "config/Environment.h"
#include "network/NetworkManager.h"
#include "services/PcMetrics.h"

class PcMetricsService {
   public:
    PcMetricsService(NetworkManager &networkManager);
    bool fetchData(PcMetrics &outData);

   private:
    bool parseData(const String &rawData, PcMetrics &outData);

    NetworkManager &networkManager_;
};

#endif