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

    Serial.print("Free heap memory after deserialization: ");
    Serial.println(freeHeap);

    Serial.print("Memory usage: ");
    Serial.println(doc.memoryUsage());

    // Navigate through the JSON structure and extract data
    const JsonArrayConst &childrenPC = doc["Children"][0]["Children"];

    // Motherboard data
    const JsonArrayConst &childrenMB = childrenPC[0]["Children"][0]["Children"];
    outData.cpu_temperature = atof(childrenMB[1]["Children"][0]["Value"]);
    outData.cpu_fan = atoi(childrenMB[2]["Children"][0]["Value"]);
    outData.front_fan = atoi(childrenMB[2]["Children"][2]["Value"]);
    outData.back_fan = atoi(childrenMB[2]["Children"][6]["Value"]);

    // CPU data
    const JsonArrayConst &childrenCPU = childrenPC[1]["Children"];
    outData.cpu_power = static_cast<uint16_t>(atof(childrenCPU[1]["Children"][0]["Value"]) + 0.5);
    outData.cpu_load = static_cast<uint8_t>(atof(childrenCPU[4]["Children"][0]["Value"]) + 0.5);

    Serial.println(outData.cpu_load);

    for (int i = 1; i < 21; i++) {
        outData.cpu_thread_load[i - 1] = atof(childrenCPU[4]["Children"][i]["Value"]);
    }

    // Memory data
    const JsonArrayConst &childrenRAM = childrenPC[2]["Children"][0]["Children"];
    outData.mem_load = static_cast<uint8_t>(atof(childrenRAM[0]["Value"]) + 0.5);

    Serial.print("MEM Load: ");
    Serial.println(outData.mem_load);

    // GPU data
    const JsonArrayConst &childrenGPU = childrenPC[3]["Children"];
    //outData.gpu_power = static_cast<uint16_t>(atof(childrenGPU[1]["Children"][0]["Value"]) + 0.5);
    // outData.gpu_temperature = atof(childrenGPU[3]["Children"][0]["Value"]);
    // outData.gpu_load = static_cast<uint8_t>(atof(childrenGPU[4]["Children"][0]["Value"]) + 0.5);
    outData.gpu_3d = static_cast<uint8_t>(atof(childrenGPU[0]["Children"][0]["Value"]) + 0.5);
    outData.gpu_compute = static_cast<uint8_t>(atof(childrenGPU[0]["Children"][1]["Value"]) + 0.5);
    // outData.gpu_decode = static_cast<uint8_t>(atof(childrenGPU[4]["Children"][8]["Value"]) + 0.5);
    // outData.gpu_fan = atoi(childrenGPU[5]["Children"][0]["Value"]);
    outData.gpu_mem = atof(childrenGPU[1]["Children"][0]["Value"]) / 163.84;

    outData.gpu_load = 50; // For testing purposes

    Serial.print("GPU 3D: ");
    Serial.println(outData.gpu_3d);
    Serial.print("GPU Compute: ");
    Serial.println(outData.gpu_compute);
    Serial.print("GPU Memory: ");
    Serial.println(outData.gpu_mem);

    // Ethernet data
    const JsonArrayConst &childrenETH = childrenPC[12]["Children"];
    const JsonArrayConst &childrenETHThroughput = childrenETH[2]["Children"];
    //outData.eth_up = this->parseNetworkSpeed(childrenETHThroughput[0]["Value"]);
    //outData.eth_dn = this->parseNetworkSpeed(childrenETHThroughput[1]["Value"]);

    Serial.println("Ethernet parsed successfully");

    // Mark data as available and updated
    outData.is_available = true;
    outData.is_updated = true;

    Serial.println("Parsing completed successfully.");

    return true;
}