#ifndef PCMETRICS_SERVICE_H
#define PCMETRICS_SERVICE_H

#include "config/Environment.h"
#include "core/network/HttpDownloader.h"
#include "services/PcMetrics.h"

class PcMetricsService {
   public:
    PcMetricsService();
    bool fetchData(PcMetrics &outData);

   private:
    bool parseData(const String &rawData, PcMetrics &outData);
    HttpDownloader downloader_;
};

#endif