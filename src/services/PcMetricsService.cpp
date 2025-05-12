#include "PcMetricsService.h"

PcMetricsService::PcMetricsService(NetworkManager &networkManager)
: networkManager_(networkManager) {};

bool PcMetricsService::fetchData(PcMetrics &outData) {
    String rawData;
    if (networkManager_.getHttpClient().download(OHM_API, rawData)) {
        return parseData(rawData, outData);
    } else {
        outData.is_available = false;
        Serial.println("Failed to fetch data from API");
    }
    return false;
}

bool PcMetricsService::parseData(const String &rawData, PcMetrics &outData) {
    // Check available heap memory
    size_t freeHeap = ESP.getFreeHeap();
    Serial.print("Free heap memory: ");
    Serial.println(freeHeap);

    Serial.print("Raw JSON size: ");
    Serial.println(rawData.length());

    DynamicJsonDocument doc(64536);

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(doc, rawData);
    if (error) {
        outData.is_available = false;
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return false;
    }

    Serial.print("Memory usage: ");
    Serial.println(doc.memoryUsage());

    // ...existing JSON parsing logic...
    const JsonArrayConst &childrenPC = doc["Children"][0]["Children"];
    // Parse other fields as before...

    // Mark data as available and updated
    outData.is_available = true;
    outData.is_updated = true;

    return true;
}