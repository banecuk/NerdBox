#include "PcMetricsService.h"

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
    // Create a filter to only parse the fields we need for better performance
    filter_["Metrics"]["Cpu"]["Load"] = true;
    filter_["Metrics"]["Cpu"]["TemperatureCoreMax"] = true;
    filter_["Metrics"]["Cpu"]["PackagePower"] = true;
    filter_["Metrics"]["Cpu"]["CoreLoads"] = true;

    filter_["Metrics"]["Ram"]["Load"] = true;

    filter_["Metrics"]["Gpu"]["Load"] = true;
    filter_["Metrics"]["Gpu"]["Temperature"] = true;
    filter_["Metrics"]["Gpu"]["PackagePower"] = true;
    filter_["Metrics"]["Gpu"]["Fan"] = true;
    filter_["Metrics"]["Gpu"]["D3d3d"] = true;
    filter_["Metrics"]["Gpu"]["D3dCompute"] = true;

    filter_["Metrics"]["Motherboard"]["CpuFan"] = true;
    filter_["Metrics"]["Motherboard"]["SystemFans"] = true;

    filter_["Metrics"]["Timestamp"] = true;
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    String rawData;
    if (networkManager_.getHttpClient().download(LIBRE_HM_API, rawData)) {
        return parseData(rawData, outData);
    } else {
        outData.is_available = false;
        logger_.error("Failed to fetch data from PC metrics API");
        return false;
    }
}

bool PcMetricsService::parseData(const String& rawData, PcMetrics& outData) {
    unsigned long startTime = millis();
    bool allComponentsValid = true;

    JsonDocument doc;
    DeserializationError error =
        deserializeJson(doc, rawData, DeserializationOption::Filter(filter_),
                        DeserializationOption::NestingLimit(12));

    if (error) {
        logger_.errorf("JSON parsing failed: %s", error.c_str());
        outData.is_available = false;
        return false;
    }

    JsonObject metrics = doc["Metrics"];
    if (metrics.isNull()) {
        logger_.error("No Metrics object found in JSON");
        allComponentsValid = false;
    }

    // Parse CPU data
    JsonObject cpu = metrics["Cpu"];
    if (!cpu.isNull()) {
        outData.cpu_load = static_cast<uint8_t>(cpu["Load"] | 0.0f);
        outData.cpu_temperature = static_cast<uint8_t>(cpu["TemperatureCoreMax"] | 0);
        outData.cpu_power = static_cast<uint16_t>(cpu["PackagePower"] | 0);

        // Parse core loads
        JsonArray coreLoads = cpu["CoreLoads"];
        if (!coreLoads.isNull()) {
            uint8_t coreCount = config_.getPcMetricsCores();
            for (int i = 0; i < coreCount && i < coreLoads.size(); i++) {
                outData.cpu_thread_load[i] = static_cast<uint8_t>(coreLoads[i] | 0);
            }
        }
    } else {
        allComponentsValid = false;
        logger_.warning("CPU data missing from metrics");
    }

    // Parse RAM data
    JsonObject ram = metrics["Ram"];
    if (!ram.isNull()) {
        outData.mem_load = static_cast<uint8_t>(ram["Load"] | 0.0f);
    } else {
        allComponentsValid = false;
        logger_.warning("RAM data missing from metrics");
    }

    // Parse GPU data
    JsonObject gpu = metrics["Gpu"];
    if (!gpu.isNull()) {
        outData.gpu_temperature = static_cast<uint8_t>(gpu["Temperature"] | 0);
        outData.gpu_3d = static_cast<uint8_t>(gpu["D3d3d"] | 0);
        outData.gpu_compute = static_cast<uint8_t>(gpu["D3dCompute"] | 0);
        outData.gpu_fan = static_cast<uint16_t>(gpu["Fan"] | 0);

        // Use GPU PackagePower if available, otherwise use CPU PackagePower
        if (gpu["PackagePower"].is<float>()) {  // TODO check !!!
            // GPU has its own power reading
        }
        // Note: GPU power isn't directly mapped in your PcMetrics struct
    } else {
        allComponentsValid = false;
        logger_.warning("GPU data missing from metrics");
    }

    // Parse Motherboard/Fan data
    JsonObject motherboard = metrics["Motherboard"];
    if (!motherboard.isNull()) {
        outData.cpu_fan = static_cast<uint16_t>(motherboard["CpuFan"] | 0);

        JsonArray systemFans = motherboard["SystemFans"];
        if (!systemFans.isNull()) {
            // Map system fans to available slots
            if (systemFans.size() > 0) {
                outData.front_fan = static_cast<uint16_t>(systemFans[0] | 0);
            }
            if (systemFans.size() > 4) {
                outData.back_fan = static_cast<uint16_t>(systemFans[4] | 0);
            }
        }
    } else {
        allComponentsValid = false;
        logger_.warning("Motherboard data missing from metrics");
    }

    // Update metrics
    outData.last_update_timestamp = millis();
    outData.is_available = allComponentsValid;

    if (allComponentsValid) {
        unsigned long parseTime = millis() - startTime;
        systemMetrics_.setPcMetricsJsonParseTime(parseTime);
        logger_.infof("PC metrics parsed successfully in %lu ms", parseTime);
    } else {
        logger_.warning("Some hardware components missing or failed to parse");
    }

    return allComponentsValid;
}