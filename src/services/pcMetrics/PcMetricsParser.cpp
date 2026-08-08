#include "PcMetricsParser.h"

namespace PcMetricsParser {

void buildFilter(JsonDocument& filter) {
    JsonObject root = filter.to<JsonObject>();
    JsonObject metrics = root["Metrics"].to<JsonObject>();

    JsonObject cpu = metrics["Cpu"].to<JsonObject>();
    cpu["Load"] = true;
    cpu["CoreLoads"] = true;

    JsonObject cpuExtended = metrics["CpuExtended"].to<JsonObject>();
    cpuExtended["TemperatureCoreMax"] = true;
    cpuExtended["PackagePower"] = true;

    JsonObject ram = metrics["Ram"].to<JsonObject>();
    ram["Load"] = true;

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

    JsonObject motherboard = metrics["Motherboard"].to<JsonObject>();
    motherboard["CpuFan"] = true;
    motherboard["SystemFans"] = true;

    JsonObject disks = metrics["Disks"].to<JsonObject>();
    disks["Drives"] = true;

    JsonObject network = metrics["Network"].to<JsonObject>();
    network["TotalUploadKBPerSec"] = true;
    network["TotalDownloadKBPerSec"] = true;

    root["Timestamp"] = true;
}

SectionResult parseAllSections(JsonObjectConst metrics, PcMetrics& outData,
                               uint8_t configuredCoreCount, LoggerInterface& logger) {
    SectionResult result;

    auto dispatch = [&](Section bit, const char* key, bool (*parse)(JsonObjectConst, PcMetrics&)) {
        JsonObjectConst section = metrics[key];
        if (section.isNull()) {
            return;
        }
        result.sectionsSeen |= bit;
        if (!parse(section, outData)) {
            result.sectionsFailed |= bit;
        }
    };

    // Cpu takes the extra core-count/logger arguments, so it can't go through
    // the uniform dispatch above.
    JsonObjectConst cpu = metrics["Cpu"];
    if (!cpu.isNull()) {
        result.sectionsSeen |= kCpu;
        if (!parseCpuData(cpu, outData, configuredCoreCount, logger)) {
            result.sectionsFailed |= kCpu;
        }
    }

    dispatch(kCpuExtended, "CpuExtended", &parseCpuExtendedData);
    dispatch(kRam, "Ram", &parseRamData);
    dispatch(kGpu, "Gpu", &parseGpuData);
    dispatch(kMotherboard, "Motherboard", &parseMotherboardData);
    dispatch(kDisks, "Disks", &parseDiskData);
    dispatch(kNetwork, "Network", &parseNetworkData);

    return result;
}

bool parseCpuData(JsonObjectConst cpu, PcMetrics& outData, uint8_t configuredCoreCount,
                   LoggerInterface& logger) {
    // Parse core loads into a local buffer first — ThreadsWidget reads
    // outData.cpu_thread_load from the screen task with no lock, so the
    // live array is only touched once below, in one tight commit loop,
    // instead of across the whole JSON traversal above.
    const int maxThreads = sizeof(outData.cpu_thread_load) / sizeof(outData.cpu_thread_load[0]);
    uint8_t newThreadLoad[24] = {};
    static_assert(sizeof(newThreadLoad) == sizeof(outData.cpu_thread_load),
                  "newThreadLoad must match PcMetrics::cpu_thread_load's size");
    uint8_t actualCores = 0;

    JsonArrayConst coreLoads = cpu["CoreLoads"];
    if (!coreLoads.isNull()) {
        uint8_t coreCount = configuredCoreCount;
        actualCores = min(coreCount, static_cast<uint8_t>(coreLoads.size()));
        actualCores = min(actualCores, static_cast<uint8_t>(maxThreads));

        for (int i = 0; i < actualCores; i++) {
            newThreadLoad[i] = static_cast<uint8_t>(coreLoads[i] | 0);
        }

        if (actualCores < coreCount) {
            logger.warningf("Expected %d cores but found %d", coreCount, actualCores);
        }

        // Commit — slots at/after actualCores are zeroed so a shrinking core
        // count doesn't leave stale readings from a previous, larger count.
        for (int i = 0; i < maxThreads; i++) {
            outData.cpu_thread_load[i] = (i < actualCores) ? newThreadLoad[i] : 0;
        }
    }
    // Absent CoreLoads means "unchanged" (delta mode) — the array is left
    // untouched rather than zeroed.

    if (!cpu["Load"].isNull()) {
        outData.cpu_load = static_cast<uint8_t>(cpu["Load"].as<float>());
    }
    return true;
}

bool parseCpuExtendedData(JsonObjectConst cpuExtended, PcMetrics& outData) {
    if (!cpuExtended["TemperatureCoreMax"].isNull()) {
        outData.cpu_temperature = static_cast<uint8_t>(cpuExtended["TemperatureCoreMax"].as<float>());
    }
    if (!cpuExtended["PackagePower"].isNull()) {
        outData.cpu_power = static_cast<uint16_t>(cpuExtended["PackagePower"].as<float>());
    }
    return true;
}

bool parseRamData(JsonObjectConst ram, PcMetrics& outData) {
    if (!ram["Load"].isNull()) {
        outData.mem_load = static_cast<uint8_t>(ram["Load"].as<float>());
    }
    return true;
}

bool parseGpuData(JsonObjectConst gpu, PcMetrics& outData) {
    if (!gpu["Load"].isNull()) {
        outData.gpu_load = static_cast<uint8_t>(gpu["Load"].as<float>());
    }
    if (!gpu["Temperature"].isNull()) {
        outData.gpu_temperature = static_cast<uint8_t>(gpu["Temperature"].as<float>());
    }
    if (!gpu["D3d3d"].isNull()) {
        outData.gpu_3d = static_cast<uint16_t>(gpu["D3d3d"].as<float>());
    }
    if (!gpu["D3dCompute"].isNull()) {
        outData.gpu_compute = static_cast<uint16_t>(gpu["D3dCompute"].as<float>());
    }
    if (!gpu["Fan"].isNull()) {
        outData.gpu_fan = static_cast<uint16_t>(gpu["Fan"].as<float>());
    }
    if (!gpu["PackagePower"].isNull()) {
        outData.gpu_power = static_cast<uint16_t>(gpu["PackagePower"].as<float>());
    }
    if (!gpu["MemoryLoad"].isNull()) {
        outData.gpu_mem = static_cast<uint16_t>(gpu["MemoryLoad"].as<float>());
    }
    // FullscreenFps is the one field where "absent" is a meaningful value in
    // its own right (no fullscreen app running), not just "unchanged" — the
    // polling path always sends this key on every full report, so this
    // matches existing behavior. See CLAUDE.md's float-extraction note.
    if (!gpu["FullscreenFps"].isNull()) {
        outData.gpu_fps = static_cast<int16_t>(gpu["FullscreenFps"].as<float>());
    }
    return true;
}

bool parseNetworkData(JsonObjectConst network, PcMetrics& outData) {
    // NerdWinSense reports both rates in KB/s (1 KB = 1024 bytes), matching
    // the disk read/write convention below. TotalUploadKBPerSec/
    // TotalDownloadKBPerSec are summed across all network adapters.
    if (!network["TotalUploadKBPerSec"].isNull()) {
        outData.eth_up = network["TotalUploadKBPerSec"].as<float>();
    }
    if (!network["TotalDownloadKBPerSec"].isNull()) {
        outData.eth_dn = network["TotalDownloadKBPerSec"].as<float>();
    }
    return true;
}

bool parseMotherboardData(JsonObjectConst motherboard, PcMetrics& outData) {
    if (!motherboard["CpuFan"].isNull()) {
        outData.cpu_fan = static_cast<uint16_t>(motherboard["CpuFan"].as<float>());
    }

    JsonArrayConst systemFans = motherboard["SystemFans"];
    if (!systemFans.isNull()) {
        // Build the new fan list in a local buffer first. NetworkWidget/
        // PcMetricsWidget read outData.system_fans[] and system_fan_count
        // from the screen task with no lock; committing the array in one
        // tight loop and publishing the new count last (a single word
        // write) means a reader using a stale cached count can only ever
        // see fully-written values, never a slot mid-overwrite.
        uint16_t newFans[PcMetrics::kMaxSystemFans] = {};
        uint8_t newFanCount = 0;

        for (size_t i = 0; i < systemFans.size() && newFanCount < PcMetrics::kMaxSystemFans; i++) {
            uint16_t rpm = static_cast<uint16_t>(systemFans[i] | 0);
            if (rpm > 0) {
                newFans[newFanCount++] = rpm;
            }
        }

        for (uint8_t i = 0; i < PcMetrics::kMaxSystemFans; i++) {
            outData.system_fans[i] = newFans[i];
        }
        outData.system_fan_count = newFanCount;
    }
    // Absent SystemFans means "unchanged" (delta mode) — left untouched
    // rather than cleared to zero fans.

    return true;
}

bool parseDiskData(JsonObjectConst disks, PcMetrics& outData) {
    JsonArrayConst drives = disks["Drives"];
    if (drives.isNull()) {
        // Absent Drives means "unchanged" (delta mode) — do not touch
        // disk_drives at all (in particular, never clear it based on
        // absence; only a present-and-empty array clears the list).
        return true;
    }

    PcMetricsDiskLock lock(outData);  // protect disk_drives for the duration of this write
    outData.disk_drives.clear();

    for (JsonObjectConst drive : drives) {
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

}  // namespace PcMetricsParser
