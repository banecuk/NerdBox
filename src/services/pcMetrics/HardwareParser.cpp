#include "HardwareParser.h"

// ============================================================================
// CPU Parser Implementation
// ============================================================================

ParseResult CpuParser::parse(JsonArray children, PcMetrics& out) {
    if (children.isNull()) {
        return ParseResult::Err("CPU children array is null");
    }

    bool foundLoad = false;
    bool foundPower = false;

    for (JsonObject section : children) {
        String text = section["Text"] | "";

        if (text.indexOf("Load") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parseLoadMetrics(sensors, out);
            if (result.success) {
                foundLoad = true;
            } else {
                logger_.warning("CPU: " + result.errorMessage);
            }
        } else if (text.indexOf("Power") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parsePowerMetrics(sensors, out);
            if (result.success) {
                foundPower = true;
            } else {
                logger_.warning("CPU: " + result.errorMessage);
            }
        }
    }

    if (!foundLoad) {
        return ParseResult::Err("CPU load metrics not found");
    }
    if (!foundPower) {
        return ParseResult::Err("CPU power metrics not found");
    }

    return ParseResult::Ok();
}

ParseResult CpuParser::parseLoadMetrics(JsonArray loadSensors, PcMetrics& out) {
    if (loadSensors.isNull() || loadSensors.size() == 0) {
        return ParseResult::Err("Load sensors array is empty");
    }

    // Parse CPU Total load (first entry)
    if (loadSensors.size() > 0) {
        out.cpu_load = parseValue<uint8_t>(loadSensors[0]["Value"], 0);
    } else {
        return ParseResult::Err("No CPU total load entry");
    }

    // Parse thread loads (skip CPU total and core max)
    size_t expectedEntries = config_.getPcMetricsCores() + CPU_LOAD_OFFSET;
    if (loadSensors.size() < expectedEntries) {
        logger_.warningf("Insufficient CPU load entries: expected %d, found %d", expectedEntries,
                         loadSensors.size());
        return ParseResult::Err("Insufficient CPU load entries");
    }

    for (size_t i = 0; i < config_.getPcMetricsCores(); ++i) {
        size_t jsonIndex = i + CPU_LOAD_OFFSET;
        out.cpu_thread_load[i] = parseValue<uint8_t>(loadSensors[jsonIndex]["Value"], 0);
    }

    return ParseResult::Ok();
}

ParseResult CpuParser::parsePowerMetrics(JsonArray powerSensors, PcMetrics& out) {
    if (powerSensors.isNull()) {
        return ParseResult::Err("Power sensors array is null");
    }

    // Look for "CPU Package" or "CPU" power sensor
    JsonObject cpuPower = SensorFinder::findByPartialMatch(powerSensors, {"CPU Package", "CPU"});

    if (cpuPower.isNull()) {
        return ParseResult::Err("CPU power sensor not found");
    }

    out.cpu_power = parseValue<uint16_t>(cpuPower["Value"], 0);
    return ParseResult::Ok();
}

// ============================================================================
// GPU Parser Implementation
// ============================================================================

ParseResult GpuParser::parse(JsonArray children, PcMetrics& out) {
    if (children.isNull()) {
        return ParseResult::Err("GPU children array is null");
    }

    bool foundLoad = false;
    bool foundMemory = false;

    for (JsonObject section : children) {
        String text = section["Text"] | "";

        if (text.indexOf("Load") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parseLoadMetrics(sensors, out);
            if (result.success) {
                foundLoad = true;
            } else {
                logger_.warning("GPU: " + result.errorMessage);
            }
        } else if (text.indexOf("Data") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parseMemoryMetrics(sensors, out);
            if (result.success) {
                foundMemory = true;
            } else {
                logger_.warning("GPU: " + result.errorMessage);
            }
        }
    }

    if (!foundLoad) {
        return ParseResult::Err("GPU load metrics not found");
    }
    if (!foundMemory) {
        return ParseResult::Err("GPU memory metrics not found");
    }

    return ParseResult::Ok();
}

ParseResult GpuParser::parseLoadMetrics(JsonArray loadSensors, PcMetrics& out) {
    if (loadSensors.isNull()) {
        return ParseResult::Err("Load sensors array is null");
    }

    // Find D3D 3D or GPU Core load
    JsonObject gpu3d = SensorFinder::findByPartialMatch(loadSensors, {"D3D 3D", "GPU Core"});
    if (!gpu3d.isNull()) {
        out.gpu_3d = parseValue<uint8_t>(gpu3d["Value"], 0);
    }

    // Find D3D Compute or Compute load
    JsonObject gpuCompute =
        SensorFinder::findByPartialMatch(loadSensors, {"D3D Compute", "Compute"});
    if (!gpuCompute.isNull()) {
        out.gpu_compute = parseValue<uint8_t>(gpuCompute["Value"], 0);
    }

    if (gpu3d.isNull() && gpuCompute.isNull()) {
        return ParseResult::Err("No GPU load sensors found");
    }

    return ParseResult::Ok();
}

ParseResult GpuParser::parseMemoryMetrics(JsonArray dataSensors, PcMetrics& out) {
    if (dataSensors.isNull()) {
        return ParseResult::Err("Data sensors array is null");
    }

    // Find GPU Memory Used
    JsonObject memUsed =
        SensorFinder::findByPartialMatch(dataSensors, {"GPU Memory Used", "Memory Used"});

    if (memUsed.isNull()) {
        return ParseResult::Err("GPU memory sensor not found");
    }

    float memUsedMB = parseValue<float>(memUsed["Value"], 0.0f);
    out.gpu_mem = static_cast<uint8_t>((memUsedMB / GPU_MEMORY_CAPACITY_MB) * 100.0f);

    return ParseResult::Ok();
}

// ============================================================================
// Motherboard Parser Implementation
// ============================================================================

ParseResult MotherboardParser::parse(JsonArray children, PcMetrics& out) {
    if (children.isNull()) {
        return ParseResult::Err("Motherboard children array is null");
    }

    // Motherboard typically has a chip (e.g., Nuvoton) as first child
    if (children.size() == 0) {
        return ParseResult::Err("No motherboard chip found");
    }

    JsonArray chipChildren = children[0]["Children"];
    if (chipChildren.isNull()) {
        return ParseResult::Err("Motherboard chip has no children");
    }

    bool foundTemp = false;
    bool foundFans = false;

    for (JsonObject section : chipChildren) {
        String text = section["Text"] | "";

        if (text.indexOf("Temperature") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parseTemperatures(sensors, out);
            if (result.success) {
                foundTemp = true;
            } else {
                logger_.warning("Motherboard: " + result.errorMessage);
            }
        } else if (text.indexOf("Fan") >= 0) {
            JsonArray sensors = section["Children"];
            auto result = parseFans(sensors, out);
            if (result.success) {
                foundFans = true;
            } else {
                logger_.warning("Motherboard: " + result.errorMessage);
            }
        }
    }

    if (!foundTemp) {
        return ParseResult::Err("Motherboard temperature not found");
    }
    if (!foundFans) {
        return ParseResult::Err("Motherboard fans not found");
    }

    return ParseResult::Ok();
}

ParseResult MotherboardParser::parseTemperatures(JsonArray tempSensors, PcMetrics& out) {
    if (tempSensors.isNull()) {
        return ParseResult::Err("Temperature sensors array is null");
    }

    JsonObject cpuTemp = SensorFinder::findContaining(tempSensors, "CPU");
    if (cpuTemp.isNull()) {
        return ParseResult::Err("CPU temperature sensor not found");
    }

    out.cpu_temperature = parseValue<uint8_t>(cpuTemp["Value"], 0);
    return ParseResult::Ok();
}

ParseResult MotherboardParser::parseFans(JsonArray fanSensors, PcMetrics& out) {
    if (fanSensors.isNull()) {
        return ParseResult::Err("Fan sensors array is null");
    }

    // Find CPU Fan
    JsonObject cpuFan = SensorFinder::findContaining(fanSensors, "CPU Fan");
    if (!cpuFan.isNull()) {
        out.cpu_fan = parseValue<uint16_t>(cpuFan["Value"], 0);
    }

    // Find front fan (System Fan #1 or Front)
    JsonObject frontFan = SensorFinder::findByPartialMatch(fanSensors, {"System Fan #1", "Front"});
    if (!frontFan.isNull()) {
        out.front_fan = parseValue<uint16_t>(frontFan["Value"], 0);
    }

    // Find back fan (System Fan #5 or Back)
    JsonObject backFan = SensorFinder::findByPartialMatch(fanSensors, {"System Fan #5", "Back"});
    if (!backFan.isNull()) {
        out.back_fan = parseValue<uint16_t>(backFan["Value"], 0);
    }

    if (cpuFan.isNull() && frontFan.isNull() && backFan.isNull()) {
        return ParseResult::Err("No fan sensors found");
    }

    return ParseResult::Ok();
}

// ============================================================================
// Memory Parser Implementation
// ============================================================================

ParseResult MemoryParser::parse(JsonArray children, PcMetrics& out) {
    if (children.isNull()) {
        return ParseResult::Err("Memory children array is null");
    }

    for (JsonObject section : children) {
        String text = section["Text"] | "";

        if (text.indexOf("Load") >= 0) {
            JsonArray sensors = section["Children"];
            if (sensors.isNull()) {
                return ParseResult::Err("Memory load sensors array is null");
            }

            JsonObject memLoad = SensorFinder::findContaining(sensors, "Memory");
            if (memLoad.isNull()) {
                return ParseResult::Err("Memory load sensor not found");
            }

            out.mem_load = parseValue<uint8_t>(memLoad["Value"], 0);
            return ParseResult::Ok();
        }
    }

    return ParseResult::Err("Memory load section not found");
}