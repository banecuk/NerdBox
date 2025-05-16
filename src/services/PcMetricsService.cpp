// src/services/PcMetricsService.cpp
#include "PcMetricsService.h"
#include <cstdlib> // For strtod

PcMetricsService::PcMetricsService(NetworkManager &networkManager)
    : networkManager_(networkManager) {
    initFilter();
}

void PcMetricsService::initFilter() {
    JsonObject filter_Children_0 = filter_["Children"].add<JsonObject>();
    filter_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0 = filter_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0 = filter_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0_Children_0 = filter_Children_0_Children_0_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Children"] = true;
}

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

// Template function to parse JSON values safely
template<typename T>
T parseValue(JsonVariant value, T defaultValue) {
    if (value.isNull()) {
        return defaultValue;
    }
    if (value.is<float>() || value.is<int>()) {
        return value.as<T>();
    }
    // Handle string values (e.g., "45.0 %", "40.5 °C", "2149.0 MB")
    String str = value.as<String>();
    str.replace("%", "");
    str.replace(" °C", "");
    str.replace(" RPM", "");
    str.replace(" W", "");
    str.replace(" V", "");
    str.replace(" MB", "");
    str.replace(" GB", "");
    str.trim();
    char* endPtr;
    float result = strtod(str.c_str(), &endPtr);
    if (endPtr == str.c_str()) {
        return defaultValue; // Conversion failed
    }
    return static_cast<T>(result);
}

bool PcMetricsService::parseData(const String &rawData, PcMetrics &outData) {
    // Start timing
    unsigned long startTime = millis();

    // Clear outData to ensure no stale values
    outData = PcMetrics();

    JsonDocument doc;

    // Deserialize with filter
    DeserializationError error = deserializeJson(doc, rawData, DeserializationOption::Filter(filter_));
    if (error) {
        outData.is_available = false;
        Serial.printf("JSON deserialization failed: %s\n", error.c_str());
        return false;
    }

    // Validate top-level structure
    JsonArray children = doc["Children"];
    if (children.isNull() || children.size() == 0) {
        outData.is_available = false;
        Serial.println("Invalid JSON structure: Missing or empty Children array");
        return false;
    }

    // Navigate to hardware sections
    JsonArray childrenPC = children[0]["Children"];
    if (childrenPC.isNull()) {
        outData.is_available = false;
        Serial.println("Invalid JSON structure: Missing Children[0].Children");
        return false;
    }

    bool dataValid = true;

    // Find sections (flexible matching)
    int mbIndex = -1, cpuIndex = -1, ramIndex = -1, gpuIndex = -1;
    for (size_t i = 0; i < childrenPC.size(); i++) {
        String text = childrenPC[i]["Text"] | "";
        if (text.indexOf("Z790") >= 0) mbIndex = i;
        else if (text.indexOf("Intel Core") >= 0) cpuIndex = i;
        else if (text.indexOf("Generic Memory") >= 0) ramIndex = i;
        else if (text.indexOf("AMD Radeon") >= 0) gpuIndex = i;
    }

    // Motherboard data
    if (mbIndex >= 0) {
        JsonArray mbChildren = childrenPC[mbIndex]["Children"][0]["Children"]; // Assume chip (e.g., Nuvoton)
        bool foundTemp = false, foundFans = false;
        for (JsonObject mbChild : mbChildren) {
            String text = mbChild["Text"] | "";
            if (text.indexOf("Temperature") >= 0) {
                JsonArray temps = mbChild["Children"];
                for (JsonObject temp : temps) {
                    String tempText = temp["Text"] | "";
                    if (tempText.indexOf("CPU") >= 0) {
                        outData.cpu_temperature = parseValue(temp["Value"], 0.0f);
                        foundTemp = true;
                        break;
                    }
                }
            } else if (text.indexOf("Fan") >= 0) {
                JsonArray fans = mbChild["Children"];
                for (JsonObject fan : fans) {
                    String fanText = fan["Text"] | "";
                    if (fanText.indexOf("CPU Fan") >= 0) {
                        outData.cpu_fan = parseValue(fan["Value"], 0);
                    } else if (fanText.indexOf("System Fan #1") >= 0 || fanText.indexOf("Front") >= 0) {
                        outData.front_fan = parseValue(fan["Value"], 0);
                    } else if (fanText.indexOf("System Fan #5") >= 0 || fanText.indexOf("Back") >= 0) {
                        outData.back_fan = parseValue(fan["Value"], 0);
                    }
                }
                foundFans = true;
            }
        }
        if (!foundTemp || !foundFans) {
            dataValid = false;
        }
    } else {
        dataValid = false;
    }

    // CPU data
    if (cpuIndex >= 0) {
        JsonArray cpuChildren = childrenPC[cpuIndex]["Children"];
        bool foundPower = false, foundLoad = false;
        for (JsonObject cpuChild : cpuChildren) {
            String text = cpuChild["Text"] | "";
            if (text.indexOf("Power") >= 0) {
                JsonArray powers = cpuChild["Children"];
                for (JsonObject power : powers) {
                    String powerText = power["Text"] | "";
                    if (powerText.indexOf("CPU Package") >= 0 || powerText.indexOf("CPU") >= 0) {
                        outData.cpu_power = parseValue(power["Value"], 0.0f);
                        foundPower = true;
                        break;
                    }
                }
            } else if (text.indexOf("Load") >= 0) {
                JsonArray loads = cpuChild["Children"];
                for (JsonObject load : loads) {
                    String loadText = load["Text"] | "";
                    if (loadText.indexOf("CPU Total") >= 0 || loadText.indexOf("CPU") >= 0) {
                        outData.cpu_load = parseValue(load["Value"], 0.0f);
                        foundLoad = true;
                    } else if (loadText.startsWith("CPU Core #") && loadText.indexOf("Thread") >= 0) {
                        int coreNum = atoi(loadText.substring(10, loadText.indexOf(" Thread")).c_str()) - 1;
                        if (coreNum >= 0 && coreNum < 20) {
                            outData.cpu_thread_load[coreNum] = parseValue(load["Value"], 0.0f);
                        }
                    }
                }
            }
        }
        if (!foundPower || !foundLoad) {
            dataValid = false;
        }
    } else {
        dataValid = false;
    }

    // Memory data
    if (ramIndex >= 0) {
        JsonArray ramChildren = childrenPC[ramIndex]["Children"];
        bool foundLoad = false;
        for (JsonObject ramChild : ramChildren) {
            String text = ramChild["Text"] | "";
            if (text.indexOf("Load") >= 0) {
                JsonArray loads = ramChild["Children"];
                for (JsonObject load : loads) {
                    String loadText = load["Text"] | "";
                    if (loadText.indexOf("Memory") >= 0) {
                        outData.mem_load = parseValue(load["Value"], 0.0f);
                        foundLoad = true;
                        break;
                    }
                }
                break;
            }
        }
        if (!foundLoad) {
            dataValid = false;
        }
    } else {
        dataValid = false;
    }

    // GPU data
    if (gpuIndex >= 0) {
        JsonArray gpuChildren = childrenPC[gpuIndex]["Children"];
        bool foundLoad = false, foundMem = false;
        for (JsonObject gpuChild : gpuChildren) {
            String text = gpuChild["Text"] | "";
            if (text.indexOf("Load") >= 0) {
                JsonArray loads = gpuChild["Children"];
                for (JsonObject load : loads) {
                    String loadText = load["Text"] | "";
                    if (loadText.indexOf("D3D 3D") >= 0 || loadText.indexOf("GPU Core") >= 0) {
                        outData.gpu_3d = parseValue(load["Value"], 0.0f);
                    } else if (loadText.indexOf("D3D Compute") >= 0 || loadText.indexOf("Compute") >= 0) {
                        outData.gpu_compute = parseValue(load["Value"], 0.0f);
                    }
                }
                foundLoad = true;
            } else if (text.indexOf("Data") >= 0 || text.indexOf("Memory") >= 0) {
                JsonArray data = gpuChild["Children"];
                for (JsonObject datum : data) {
                    String dataText = datum["Text"] | "";
                    if (dataText.indexOf("GPU Memory Used") >= 0 || dataText.indexOf("Memory Used") >= 0) {
                        float memUsed = parseValue(datum["Value"], 0.0f);
                        outData.gpu_mem = (memUsed / 16368.0) * 100.0; // Convert MB to % of 16 GB
                        foundMem = true;
                        break;
                    }
                }
            }
        }
        if (!foundLoad || !foundMem) {
            dataValid = false;
        }
    } else {
        dataValid = false;
    }

    // Set availability
    outData.is_available = dataValid;
    outData.last_update_timestamp = millis();

    // Log parsing time and result
    unsigned long parseTime = millis() - startTime;
    if (dataValid) {
        Serial.printf("Parsing time: %lu ms\n", parseTime);
    } else {
        Serial.println("Warning: Missing some hardware data");
    }
    return dataValid;
}