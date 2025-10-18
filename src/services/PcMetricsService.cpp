#include "PcMetricsService.h"

#include <cstdlib>  // For strtod

namespace {
constexpr size_t CPU_LOAD_OFFSET = 2;               // Skip CPU total and core max
constexpr float GPU_MEMORY_CAPACITY_MB = 16368.0f;  // 16GB GPU
}  // namespace

PcMetricsService::PcMetricsService(NetworkManager& networkManager,
                                   ApplicationMetrics& systemMetrics, LoggerInterface& logger,
                                   AppConfigInterface& config)
    : networkManager_(networkManager),
      systemMetrics_(systemMetrics),
      logger_(logger),
      config_(config) {
    initFilter();
}

void PcMetricsService::initFilter() {
    JsonObject filter_Children_0 = filter_["Children"].add<JsonObject>();
    filter_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0 = filter_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0 =
        filter_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0["Text"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0 =
        filter_Children_0_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0["Value"] = true;

    JsonObject filter_Children_0_Children_0_Children_0_Children_0_Children_0 =
        filter_Children_0_Children_0_Children_0_Children_0["Children"].add<JsonObject>();
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Text"] = true;
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Value"] = true;
    filter_Children_0_Children_0_Children_0_Children_0_Children_0["Children"] = true;
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    String rawData;
    if (networkManager_.getHttpClient().download(LIBRE_HM_API, rawData)) {
        return parseData(rawData, outData);
    } else {
        outData.is_available = false;
        Serial.println("Failed to fetch data from API");
    }
    return false;
}

template <typename T>
T parseValue(JsonVariant value, T defaultValue) {
    if (value.isNull()) {
        return defaultValue;
    }
    if (value.is<float>() || value.is<int>()) {
        float val = value.as<float>();
        if constexpr (std::is_integral_v<T>) {
            int rounded = static_cast<int>(val + 0.5f);  // Round to nearest integer
            return static_cast<T>(rounded);
        }
        return static_cast<T>(val);
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
        return defaultValue;  // Conversion failed
    }
    if constexpr (std::is_integral_v<T>) {
        int rounded = static_cast<int>(result + 0.5f);  // Round to nearest integer
        return static_cast<T>(rounded);
    }
    return static_cast<T>(result);
}

bool PcMetricsService::parseData(const String& rawData, PcMetrics& outData) {
    // Start timing
    unsigned long startTime = millis();

    outData = PcMetrics();

    JsonDocument doc;

    // Deserialize with filter
    DeserializationError error =
        deserializeJson(doc, rawData, DeserializationOption::Filter(filter_),
                        DeserializationOption::NestingLimit(12));
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
        if (text.indexOf("Z790") >= 0)
            mbIndex = i;
        else if (text.indexOf("Intel Core") >= 0)
            cpuIndex = i;
        else if (text.indexOf("Generic Memory") >= 0)
            ramIndex = i;
        else if (text.indexOf("AMD Radeon") >= 0)
            gpuIndex = i;
    }

    // Motherboard data
    if (mbIndex >= 0) {
        JsonArray mbChildren =
            childrenPC[mbIndex]["Children"][0]["Children"];  // Assume chip (e.g., Nuvoton)
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
                    } else if (fanText.indexOf("System Fan #1") >= 0 ||
                               fanText.indexOf("Front") >= 0) {
                        outData.front_fan = parseValue(fan["Value"], 0);
                    } else if (fanText.indexOf("System Fan #5") >= 0 ||
                               fanText.indexOf("Back") >= 0) {
                        outData.back_fan = parseValue(fan["Value"], 0);
                    }
                }
                foundFans = true;
            }
        }
        if (!foundTemp || !foundFans) {
            Serial.println("Warning: Missing some motherboard data");
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
                // logger_.debug("Found CPU Load section");
                JsonArray loads = cpuChild["Children"];
                if (loads.size() > 0) {
                    // Parse CPU Total load from index 0
                    JsonObject cpuTotalLoad = loads[0];
                    String loadText = cpuTotalLoad["Text"] | "";
                    outData.cpu_load = parseValue(cpuTotalLoad["Value"], 0.0f);
                    foundLoad = true;
                } else {
                    logger_.warning("CPU Load section is empty");
                    foundLoad = false;
                }
                // Parse thread loads
                if (loads.size() >= config_.getPcMetricsCores() + CPU_LOAD_OFFSET) {
                    for (size_t i = CPU_LOAD_OFFSET;
                         i < config_.getPcMetricsCores() + CPU_LOAD_OFFSET;
                         ++i) {  // skip CPU total and CPU core max
                        JsonObject load = loads[i];
                        String loadText = load["Text"] | "";
                        int threadIndex = i - CPU_LOAD_OFFSET;
                        if (threadIndex >= 0 && threadIndex < config_.getPcMetricsCores()) {
                            outData.cpu_thread_load[threadIndex] = parseValue(load["Value"], 0.0f);
                        } else {
                            logger_.warningf(
                                "Invalid thread index %d for CPU thread load at JSON "
                                "index %d",
                                threadIndex, i);
                        }
                    }
                } else {
                    logger_.warningf(
                        "Insufficient entries in CPU Load section: expected at least 22, "
                        "found %d",
                        loads.size());
                    foundLoad = false;
                }
            }
        }
        if (!foundPower || !foundLoad) {
            logger_.warning("Missing some CPU data");
            dataValid = false;
        }
    } else {
        logger_.warning("CPU section not found");
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
            Serial.println("Warning: Missing memory data");
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
                    } else if (loadText.indexOf("D3D Compute") >= 0 ||
                               loadText.indexOf("Compute") >= 0) {
                        outData.gpu_compute = parseValue(load["Value"], 0.0f);
                    }
                }
                foundLoad = true;
            } else if (text.indexOf("Data") >= 0) {
                JsonArray data = gpuChild["Children"];
                for (JsonObject datum : data) {
                    String dataText = datum["Text"] | "";
                    if (dataText.indexOf("GPU Memory Used") >= 0 ||
                        dataText.indexOf("Memory Used") >= 0) {
                        float memUsed = parseValue(datum["Value"], 0.0f);
                        outData.gpu_mem = (memUsed / GPU_MEMORY_CAPACITY_MB) * 100.0;
                        foundMem = true;
                        break;
                    }
                }
            }
        }
        if (!foundLoad || !foundMem) {
            Serial.println("Warning: Missing some GPU data");
            dataValid = false;
        }
    } else {
        dataValid = false;
    }

    // Set availability
    outData.last_update_timestamp = millis();
    outData.is_available = dataValid;

    // Log parsing time in ApplicationMetrics
    if (dataValid) {
        unsigned long parseTime = millis() - startTime;
        systemMetrics_.setPcMetricsJsonParseTime(parseTime);
        // Serial.printf("Parsing time: %lu ms\n", parseTime);
    } else {
        Serial.println("Warning: Missing some hardware data");
    }
    return dataValid;
}