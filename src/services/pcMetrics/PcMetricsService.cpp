#include "PcMetricsService.h"

PcMetricsService::PcMetricsService(NetworkManager& networkManager,
                                   ApplicationMetrics& systemMetrics, LoggerInterface& logger,
                                   AppConfigInterface& config)
    : networkManager_(networkManager),
      systemMetrics_(systemMetrics),
      logger_(logger),
      config_(config) {
    // Allocate JSON document on heap
    filterDoc_ = std::make_unique<JsonDocument>();
    initFilter();
}

void PcMetricsService::initFilter() {
    // Create filter in the heap-allocated document
    JsonObject filter = filterDoc_->to<JsonObject>();
    JsonObject metrics = filter["Metrics"].to<JsonObject>();

    // CPU filters
    JsonObject cpu = metrics["Cpu"].to<JsonObject>();
    cpu["Load"] = true;
    cpu["CoreLoads"] = true;

    // CPU extended filters
    JsonObject cpuExtended = metrics["CpuExtended"].to<JsonObject>();
    cpuExtended["TemperatureCoreMax"] = true;
    cpuExtended["PackagePower"] = true;

    // RAM filters
    JsonObject ram = metrics["Ram"].to<JsonObject>();
    ram["Load"] = true;

    // GPU filters
    JsonObject gpu = metrics["Gpu"].to<JsonObject>();
    gpu["Load"] = true;
    gpu["Temperature"] = true;
    gpu["PackagePower"] = true;
    gpu["Fan"] = true;
    gpu["D3d3d"] = true;
    gpu["D3dCompute"] = true;
    gpu["MemoryUsed"] = true;
    gpu["MemoryAvailable"] = true;
    gpu["MemoryTotal"] = true;
    gpu["MemoryLoad"] = true;

    // Motherboard filters
    JsonObject motherboard = metrics["Motherboard"].to<JsonObject>();
    motherboard["CpuFan"] = true;
    motherboard["SystemFans"] = true;

    filter["Timestamp"] = true;

    // Copy filter to stack-based filter for deserialization
    filter_ = *filterDoc_;
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    String rawData;
    if (networkManager_.getHttpClient().download(LIBRE_HM_API, rawData)) {
        bool success = parseData(rawData, outData);
        if (success) {
            lastSuccessfulFetchTime_ = millis();
        }
        return success;
    } else {
        outData.is_available = false;
        logger_.error("Failed to fetch data from PC metrics API");
        return false;
    }
}

bool PcMetricsService::parseData(const String& rawData, PcMetrics& outData) {
    unsigned long startTime = millis();

    // Use heap allocation for main JSON document
    auto doc = std::make_unique<JsonDocument>();
    if (!doc) {
        logger_.error("Failed to allocate memory for JSON document");
        outData.is_available = false;
        return false;
    }

    DeserializationError error =
        deserializeJson(*doc, rawData, DeserializationOption::Filter(filter_),
                        DeserializationOption::NestingLimit(12));

    if (error) {
        logger_.errorf("JSON parsing failed: %s", error.c_str());
        outData.is_available = false;
        return false;
    }

    JsonObject metrics = (*doc)["Metrics"];
    if (metrics.isNull()) {
        logger_.error("No Metrics object found in JSON");
        outData.is_available = false;
        return false;
    }

    // Validate JSON structure before parsing
    if (!validateJsonStructure(metrics)) {
        logger_.error("JSON structure validation failed");
        outData.is_available = false;
        return false;
    }

    bool allComponentsValid = true;

    // Parse individual components with error isolation
    JsonObject cpu = metrics["Cpu"];
    if (!cpu.isNull()) {
        if (!parseCpuData(cpu, outData)) {
            allComponentsValid = false;
            logger_.warning("CPU data parsing failed");
        }
    } else {
        allComponentsValid = false;
        logger_.warning("CPU data missing from metrics");
    }

    JsonObject cpuExtended = metrics["CpuExtended"];
    if (!cpuExtended.isNull()) {
        if (!parseCpuExtendedData(cpuExtended, outData)) {
            allComponentsValid = false;
            logger_.warning("CPU extended data parsing failed");
        }
    } else {
        allComponentsValid = false;
        logger_.warning("CPU data missing from metrics");
    }

    JsonObject ram = metrics["Ram"];
    if (!ram.isNull()) {
        if (!parseRamData(ram, outData)) {
            allComponentsValid = false;
            logger_.warning("RAM data parsing failed");
        }
    } else {
        allComponentsValid = false;
        logger_.warning("RAM data missing from metrics");
    }

    JsonObject gpu = metrics["Gpu"];
    if (!gpu.isNull()) {
        if (!parseGpuData(gpu, outData)) {
            allComponentsValid = false;
            logger_.warning("GPU data parsing failed");
        }
    } else {
        allComponentsValid = false;
        logger_.warning("GPU data missing from metrics");
    }

    JsonObject motherboard = metrics["Motherboard"];
    if (!motherboard.isNull()) {
        if (!parseMotherboardData(motherboard, outData)) {
            allComponentsValid = false;
            logger_.warning("Motherboard data parsing failed");
        }
    } else {
        allComponentsValid = false;
        logger_.warning("Motherboard data missing from metrics");
    }

    // Update metrics
    outData.last_update_timestamp = millis();
    outData.is_available = allComponentsValid;

    unsigned long parseTime = millis() - startTime;
    systemMetrics_.setPcMetricsJsonParseTime(parseTime);

    if (allComponentsValid) {
        // logger_.debugf("PC metrics parsed successfully in %lu ms", parseTime);
    } else {
        logger_.warning("Some hardware components missing or failed to parse");
    }

    // Memory is automatically freed when 'doc' goes out of scope
    return allComponentsValid;
}

bool PcMetricsService::validateJsonStructure(JsonObject metrics) {
    if (metrics.isNull())
        return false;

    // Check for required top-level objects
    if (!metrics["Cpu"].is<JsonObject>()) {
        logger_.warning("Missing or invalid CPU object in JSON");
        return false;
    }

    if (!metrics["Ram"].is<JsonObject>()) {
        logger_.warning("Missing or invalid RAM object in JSON");
        return false;
    }

    return true;
}

bool PcMetricsService::parseCpuData(JsonObject cpu, PcMetrics& outData) {
    try {
        outData.cpu_load = static_cast<uint8_t>(cpu["Load"] | 0.0f);
        outData.cpu_temperature = static_cast<uint8_t>(cpu["TemperatureCoreMax"] | 0);
        outData.cpu_power = static_cast<uint16_t>(cpu["PackagePower"] | 0);

        // Parse core loads with bounds checking
        JsonArray coreLoads = cpu["CoreLoads"];
        if (!coreLoads.isNull()) {
            uint8_t coreCount = config_.getPcMetricsCores();
            uint8_t actualCores = min(coreCount, static_cast<uint8_t>(coreLoads.size()));

            for (int i = 0; i < actualCores; i++) {
                if (i < 20) {  // Check against array size in PcMetrics
                    outData.cpu_thread_load[i] = static_cast<uint8_t>(coreLoads[i] | 0);
                } else {
                    break;  // Prevent array overflow
                }
            }

            if (actualCores < coreCount) {
                logger_.warningf("Expected %d cores but found %d", coreCount, actualCores);
            }
        }
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("CPU data parsing exception: %s", e.what());
        return false;
    }
}

bool PcMetricsService::parseCpuExtendedData(JsonObject cpuExtended, PcMetrics& outData) {
    try {
        outData.cpu_temperature = static_cast<uint8_t>(cpuExtended["TemperatureCoreMax"] | 0);
        outData.cpu_power = static_cast<uint16_t>(cpuExtended["PackagePower"] | 0.0f);
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("CPU extended data parsing exception: %s", e.what());
        return false;
    }
}

bool PcMetricsService::parseRamData(JsonObject ram, PcMetrics& outData) {
    try {
        outData.mem_load = static_cast<uint8_t>(ram["Load"] | 0.0f);
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("RAM data parsing exception: %s", e.what());
        return false;
    }
}

bool PcMetricsService::parseGpuData(JsonObject gpu, PcMetrics& outData) {
    try {
        outData.gpu_load = static_cast<uint8_t>(gpu["Load"] | 0);
        outData.gpu_temperature = static_cast<uint8_t>(gpu["Temperature"] | 0);
        outData.gpu_3d = static_cast<uint16_t>(gpu["D3d3d"] | 0);
        outData.gpu_compute = static_cast<uint16_t>(gpu["D3dCompute"] | 0);
        outData.gpu_fan = static_cast<uint16_t>(gpu["Fan"] | 0);
        outData.gpu_power = static_cast<uint16_t>(gpu["PackagePower"] | 0);
        outData.gpu_mem = static_cast<uint16_t>(gpu["MemoryLoad"] | 0.0f);
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("GPU data parsing exception: %s", e.what());
        return false;
    }
}

bool PcMetricsService::parseMotherboardData(JsonObject motherboard, PcMetrics& outData) {
    try {
        outData.cpu_fan = static_cast<uint16_t>(motherboard["CpuFan"] | 0);

        JsonArray systemFans = motherboard["SystemFans"];
        if (!systemFans.isNull()) {
            for (int i = 0; i < systemFans.size(); i++) {
                outData.system_fans[i] = static_cast<uint16_t>(systemFans[i] | 0);
            }
        }
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("Motherboard data parsing exception: %s", e.what());
        return false;
    }
}