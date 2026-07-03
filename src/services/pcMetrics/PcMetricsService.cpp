#include "PcMetricsService.h"

PcMetricsService::PcMetricsService(NetworkManager& networkManager,
                                   ApplicationMetrics& systemMetrics, LoggerInterface& logger,
                                   AppConfigInterface& config)
    : networkManager_(networkManager),
      systemMetrics_(systemMetrics),
      logger_(logger),
      config_(config) {
    doc_ = std::make_unique<JsonDocument>();
    initFilter();
}

void PcMetricsService::initFilter() {
    JsonObject filter = filter_.to<JsonObject>();
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

    JsonObject disks = metrics["Disks"].to<JsonObject>();
    disks["Drives"] = true;

    filter["Timestamp"] = true;
}

bool PcMetricsService::fetchData(PcMetrics& outData) {
    // Deserializes straight from the HTTP stream into doc_ — no intermediate
    // String buffer for the response body — and reuses doc_ across fetches
    // to avoid heap fragmentation.
    doc_->clear();
    HttpClient& http = networkManager_.getHttpClient();
    if (!http.downloadAndParse(LIBRE_HM_API, *doc_, filter_)) {
        outData.is_available = false;
        if (http.getLastHttpCode() == HTTP_CODE_OK) {
            logger_.errorf("JSON parsing failed: %s", http.getLastParseError().c_str());
        } else {
            logger_.error("Failed to fetch data from PC metrics API");
        }
        return false;
    }

    return parseData(outData);
}

bool PcMetricsService::parseData(PcMetrics& outData) {
    unsigned long startTime = millis();

    JsonObject metrics = (*doc_)["Metrics"];
    if (metrics.isNull()) {
        logger_.error("No Metrics object found in JSON");
        outData.is_available = false;
        return false;
    }

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

    JsonObject disks = metrics["Disks"];
    if (!disks.isNull()) {
        if (!parseDiskData(disks, outData)) {
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

bool PcMetricsService::parseDiskData(JsonObject disks, PcMetrics& outData) {
    PcMetricsDiskLock lock(outData);  // protect disk_drives for the duration of this write
    outData.disk_drives.clear();

    JsonArray drives = disks["Drives"];
    if (drives.isNull()) {
        return true;
    }

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
}

bool PcMetricsService::parseCpuData(JsonObject cpu, PcMetrics& outData) {
    const uint8_t newCpuLoad = static_cast<uint8_t>(cpu["Load"] | 0.0f);
    // Note: cpu_temperature and cpu_power come from CpuExtended, not Cpu

    // Parse core loads into a local buffer first — ThreadsWidget reads
    // outData.cpu_thread_load from the screen task with no lock, so the
    // live array is only touched once below, in one tight commit loop,
    // instead of across the whole JSON traversal above.
    const int maxThreads = sizeof(outData.cpu_thread_load) / sizeof(outData.cpu_thread_load[0]);
    uint8_t newThreadLoad[24] = {};
    static_assert(sizeof(newThreadLoad) == sizeof(outData.cpu_thread_load),
                  "newThreadLoad must match PcMetrics::cpu_thread_load's size");
    uint8_t actualCores = 0;

    JsonArray coreLoads = cpu["CoreLoads"];
    if (!coreLoads.isNull()) {
        uint8_t coreCount = config_.getPcMetricsCores();
        actualCores = min(coreCount, static_cast<uint8_t>(coreLoads.size()));
        actualCores = min(actualCores, static_cast<uint8_t>(maxThreads));

        for (int i = 0; i < actualCores; i++) {
            newThreadLoad[i] = static_cast<uint8_t>(coreLoads[i] | 0);
        }

        if (actualCores < coreCount) {
            logger_.warningf("Expected %d cores but found %d", coreCount, actualCores);
        }
    }

    // Commit — cpu_load and the whole thread-load array update together.
    // Slots at/after actualCores are zeroed so a shrinking core count
    // doesn't leave stale readings from a previous, larger core count.
    outData.cpu_load = newCpuLoad;
    if (!coreLoads.isNull()) {
        for (int i = 0; i < maxThreads; i++) {
            outData.cpu_thread_load[i] = (i < actualCores) ? newThreadLoad[i] : 0;
        }
    }
    return true;
}

bool PcMetricsService::parseCpuExtendedData(JsonObject cpuExtended, PcMetrics& outData) {
    outData.cpu_temperature = static_cast<uint8_t>(cpuExtended["TemperatureCoreMax"] | 0);
    outData.cpu_power = static_cast<uint16_t>(cpuExtended["PackagePower"] | 0.0f);
    return true;
}

bool PcMetricsService::parseRamData(JsonObject ram, PcMetrics& outData) {
    outData.mem_load = static_cast<uint8_t>(ram["Load"] | 0.0f);
    return true;
}

bool PcMetricsService::parseGpuData(JsonObject gpu, PcMetrics& outData) {
    outData.gpu_load = static_cast<uint8_t>(gpu["Load"] | 0);
    outData.gpu_temperature = static_cast<uint8_t>(gpu["Temperature"] | 0);
    outData.gpu_3d = gpu["D3d3d"].isNull() ? 0 : static_cast<uint16_t>(gpu["D3d3d"].as<float>());
    outData.gpu_compute =
        gpu["D3dCompute"].isNull() ? 0 : static_cast<uint16_t>(gpu["D3dCompute"].as<float>());
    outData.gpu_fan = static_cast<uint16_t>(gpu["Fan"] | 0);
    outData.gpu_power = static_cast<uint16_t>(gpu["PackagePower"] | 0);
    outData.gpu_mem = static_cast<uint16_t>(gpu["MemoryLoad"] | 0.0f);
    outData.gpu_fps = gpu["FullscreenFps"].isNull()
                          ? int16_t(-1)
                          : static_cast<int16_t>(gpu["FullscreenFps"].as<float>());
    return true;
}

bool PcMetricsService::parseMotherboardData(JsonObject motherboard, PcMetrics& outData) {
    outData.cpu_fan = static_cast<uint16_t>(motherboard["CpuFan"] | 0);

    // Build the new fan list in a local buffer first. NetworkWidget/
    // PcMetricsWidget read outData.system_fans[] and system_fan_count
    // from the screen task with no lock; the old code reset
    // system_fan_count to 0 up front and incremented it while writing
    // system_fans[] in the same loop, so a reader holding a stale
    // (pre-reset) count could index into a slot being actively
    // overwritten for the new list.
    JsonArray systemFans = motherboard["SystemFans"];
    uint16_t newFans[PcMetrics::kMaxSystemFans] = {};
    uint8_t newFanCount = 0;

    if (!systemFans.isNull()) {
        for (int i = 0; i < systemFans.size() && newFanCount < PcMetrics::kMaxSystemFans; i++) {
            uint16_t rpm = static_cast<uint16_t>(systemFans[i] | 0);
            if (rpm > 0) {
                newFans[newFanCount++] = rpm;
            }
        }
    }

    // Commit the array in one tight loop, then publish the new count
    // last (a single word write) — a reader using a stale cached count
    // can therefore only ever see fully-written values, never a slot
    // mid-overwrite for the new list.
    for (uint8_t i = 0; i < PcMetrics::kMaxSystemFans; i++) {
        outData.system_fans[i] = newFans[i];
    }
    outData.system_fan_count = newFanCount;
    return true;
}