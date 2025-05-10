#include "PcMetricsService.h"

PcMetricsService::PcMetricsService(NetworkManager &networkManager)
: networkManager_(networkManager) {};

bool PcMetricsService::fetchData(PcMetrics &outData) {
    String rawData;
    if (networkManager_.getHttpDownloader().download(OHM_API, rawData)) {
        return parseData(rawData, outData);
    }
    return false;
}

bool PcMetricsService::parseData(const String &rawData,
                                       PcMetrics &outData) {
    // Implement your parsing logic here
    // Example:
    // JsonDocument doc;
    // deserializeJson(doc, rawData);
    // outData.temperature = doc["temperature"];
    // outData.humidity = doc["humidity"];

    outData.cpu_load = 50;
    outData.gpu_load = 25;

    // outData.is_available = true;
    outData.is_updated = true;

    return true;  // TODO false if parsing fails


    // StaticJsonDocument<512> doc;
    // DeserializationError error = deserializeJson(doc, rawData);
    // if (error) {
    //     logger_.error("JSON parsing failed: " + String(error.c_str()));
    //     return false;
    // }
    // outData.cpu_load = doc["cpu_load"] | 0;
    // outData.gpu_load = doc["gpu_load"] | 0;
    // outData.is_updated = true;
    // return true;
}