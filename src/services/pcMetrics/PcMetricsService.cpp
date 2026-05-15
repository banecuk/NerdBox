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
    doc_ = std::make_unique<JsonDocument>();
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
    gpu["FullscreenFps"] = true;

    // Motherboard filters
    JsonObject motherboard = metrics["Motherboard"].to<JsonObject>();
    motherboard["CpuFan"] = true;
    motherboard["SystemFans"] = true;

    // SIMPLE DISK FILTER - Remove disk filtering entirely for now
    // We'll parse disks without filtering to avoid issues
    JsonObject disks = metrics["Disks"].to<JsonObject>();
    disks["Drives"] = true;  // Simple approach - include all disk data

    filter["Timestamp"] = true;

    // Copy filter to stack-based filter for deserialization
    filter_ = *filterDoc_;
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    if (!filterInitialized_) {
        initFilter();
        filterInitialized_ = true;
    }

    rawData_ = "";  // clear without freeing the buffer
    if (networkManager_.getHttpClient().download(LIBRE_HM_API, rawData_)) {
        bool success = parseData(rawData_, outData);
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

    // Reuse the persistent doc_ to avoid heap fragmentation on every fetch
    if (!doc_) {
        logger_.error("JSON document not allocated");
        outData.is_available = false;
        return false;
    }
    doc_->clear();

    DeserializationError error = deserializeJson(*doc_, rawData, DeserializationOption::Filter(filter_));

    if (error) {
        logger_.errorf("JSON parsing failed: %s", error.c_str());
        outData.is_available = false;
        return false;
    }

    JsonObject metrics = (*doc_)["Metrics"];
    if (metrics.isNull()) {
        logger_.error("No Metrics object found in JSON");
        outData.is_available = false;
        return false;
    }

    // Check if disks exist in the unfiltered JSON
    JsonObject disks = metrics["Disks"];
    // if (!disks.isNull()) {
    //     logger_.debug("✅ Disks found in JSON (no filter)");
    // }

    bool allComponentsValid = true;

    // Parse individual components with error isolation
    JsonObject cpu = metrics["Cpu"];
    if (!cpu.isNull()) {
        if (!parseCpuData(cpu, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject cpuExtended = metrics["CpuExtended"];
    if (!cpuExtended.isNull()) {
        if (!parseCpuExtendedData(cpuExtended, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject ram = metrics["Ram"];
    if (!ram.isNull()) {
        if (!parseRamData(ram, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject gpu = metrics["Gpu"];
    if (!gpu.isNull()) {
        if (!parseGpuData(gpu, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject motherboard = metrics["Motherboard"];
    if (!motherboard.isNull()) {
        if (!parseMotherboardData(motherboard, outData)) {
            allComponentsValid = false;
        }
    }

    JsonObject disksFiltered = metrics["Disks"];
    if (!disksFiltered.isNull()) {
        if (!parseDiskData(disksFiltered, outData)) {
            allComponentsValid = false;
        }
    } else {
        logger_.debug("No Disks in filtered JSON");
    }

    // Update metrics
    outData.last_update_timestamp = millis();
    outData.is_available = allComponentsValid;

    unsigned long parseTime = millis() - startTime;
    systemMetrics_.setPcMetricsJsonParseTime(parseTime);

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

bool PcMetricsService::parseDiskData(JsonObject disks, PcMetrics& outData) {
    try {
        PcMetricsDiskLock lock(outData);  // protect disk_drives for the duration of this write
        outData.disk_drives.clear();

        JsonArray drives = disks["Drives"];
        if (drives.isNull()) {
            return true;
        }

        // logger_.debugf("Found %d disk drives", drives.size());

        for (JsonObject drive : drives) {
            DiskDrive diskDrive;

            const char* driveName = drive["DriveName"];
            if (driveName) {
                strncpy(diskDrive.driveName, driveName, sizeof(diskDrive.driveName) - 1);
                diskDrive.driveName[sizeof(diskDrive.driveName) - 1] = '\0';
            } else {
                continue;
            }

            diskDrive.freeSpacePercent = drive["FreeSpacePercent"] | 0.0f;
            diskDrive.readKBPerSec = drive["ReadKBPerSec"] | 0.0f;
            diskDrive.writeKBPerSec = drive["WriteKBPerSec"] | 0.0f;

            outData.disk_drives.push_back(diskDrive);
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool PcMetricsService::parseCpuData(JsonObject cpu, PcMetrics& outData) {
    try {
        outData.cpu_load = static_cast<uint8_t>(cpu["Load"] | 0.0f);
        // Note: cpu_temperature and cpu_power come from CpuExtended, not Cpu

        // Parse core loads with bounds checking
        JsonArray coreLoads = cpu["CoreLoads"];
        if (!coreLoads.isNull()) {
            uint8_t coreCount = config_.getPcMetricsCores();
            uint8_t actualCores = min(coreCount, static_cast<uint8_t>(coreLoads.size()));
            const int maxThreads = sizeof(outData.cpu_thread_load) / sizeof(outData.cpu_thread_load[0]);

            for (int i = 0; i < actualCores; i++) {
                if (i < maxThreads) {  // Check against array size in PcMetrics
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
        outData.gpu_3d =
            gpu["D3d3d"].isNull() ? 0 : static_cast<uint16_t>(gpu["D3d3d"].as<float>());
        outData.gpu_compute =
            gpu["D3dCompute"].isNull() ? 0 : static_cast<uint16_t>(gpu["D3dCompute"].as<float>());
        outData.gpu_fan = static_cast<uint16_t>(gpu["Fan"] | 0);
        outData.gpu_power = static_cast<uint16_t>(gpu["PackagePower"] | 0);
        outData.gpu_mem = static_cast<uint16_t>(gpu["MemoryLoad"] | 0.0f);
        outData.gpu_fps = gpu["FullscreenFps"].isNull()
                              ? int16_t(-1)
                              : static_cast<int16_t>(gpu["FullscreenFps"].as<float>());
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
        outData.system_fan_count = 0;

        if (!systemFans.isNull()) {
            for (int i = 0; i < systemFans.size() && outData.system_fan_count < PcMetrics::kMaxSystemFans; i++) {
                uint16_t rpm = static_cast<uint16_t>(systemFans[i] | 0);
                if (rpm > 0) {
                    outData.system_fans[outData.system_fan_count++] = rpm;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        logger_.errorf("Motherboard data parsing exception: %s", e.what());
        return false;
    }
}