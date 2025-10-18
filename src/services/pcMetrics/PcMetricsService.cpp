#include "PcMetricsService.h"

#include "HardwareParser.h"

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

bool PcMetricsService::parseData(const String& rawData, PcMetrics& outData) {
    unsigned long startTime = millis();

    // Reset output data
    outData = PcMetrics();

    // Deserialize JSON with filter
    JsonDocument doc;
    DeserializationError error =
        deserializeJson(doc, rawData, DeserializationOption::Filter(filter_),
                        DeserializationOption::NestingLimit(12));

    if (error) {
        outData.is_available = false;
        logger_.errorf("JSON deserialization failed: %s", error.c_str());
        return false;
    }

    // Validate top-level structure
    JsonArray children = doc["Children"];
    if (children.isNull() || children.size() == 0) {
        outData.is_available = false;
        logger_.error("Invalid JSON: Missing or empty Children array");
        return false;
    }

    // Navigate to PC hardware sections
    JsonArray hardwareChildren = children[0]["Children"];
    if (hardwareChildren.isNull()) {
        outData.is_available = false;
        logger_.error("Invalid JSON: Missing hardware Children");
        return false;
    }

    // Find hardware component indices
    HardwareIndices indices = findHardwareIndices(hardwareChildren);

    // Parse each component
    bool allComponentsValid = true;
    allComponentsValid &= parseMotherboard(hardwareChildren, indices.motherboard, outData);
    allComponentsValid &= parseCpu(hardwareChildren, indices.cpu, outData);
    allComponentsValid &= parseMemory(hardwareChildren, indices.memory, outData);
    allComponentsValid &= parseGpu(hardwareChildren, indices.gpu, outData);

    // Update metrics
    outData.last_update_timestamp = millis();
    outData.is_available = allComponentsValid;

    if (allComponentsValid) {
        unsigned long parseTime = millis() - startTime;
        systemMetrics_.setPcMetricsJsonParseTime(parseTime);
    } else {
        logger_.warning("Some hardware components missing or failed to parse");
    }

    return allComponentsValid;
}

PcMetricsService::HardwareIndices
PcMetricsService::findHardwareIndices(JsonArray hardwareChildren) {
    HardwareIndices indices{-1, -1, -1, -1};

    for (size_t i = 0; i < hardwareChildren.size(); i++) {
        String text = hardwareChildren[i]["Text"] | "";

        if (text.indexOf("Z790") >= 0 || text.indexOf("Motherboard") >= 0) {
            indices.motherboard = i;
        } else if (text.indexOf("Intel Core") >= 0 || text.indexOf("AMD Ryzen") >= 0) {
            indices.cpu = i;
        } else if (text.indexOf("Generic Memory") >= 0 || text.indexOf("Memory") >= 0) {
            indices.memory = i;
        } else if (text.indexOf("AMD Radeon") >= 0 || text.indexOf("NVIDIA") >= 0) {
            indices.gpu = i;
        }
    }

    return indices;
}

bool PcMetricsService::parseMotherboard(JsonArray hardwareChildren, int index, PcMetrics& outData) {
    if (index < 0) {
        logger_.warning("Motherboard section not found");
        return false;
    }

    JsonArray mbChildren = hardwareChildren[index]["Children"];
    MotherboardParser parser(logger_);
    ParseResult result = parser.parse(mbChildren, outData);

    if (!result.success) {
        logger_.warning("Motherboard parse failed: " + result.errorMessage);
    }

    return result.success;
}

bool PcMetricsService::parseCpu(JsonArray hardwareChildren, int index, PcMetrics& outData) {
    if (index < 0) {
        logger_.warning("CPU section not found");
        return false;
    }

    JsonArray cpuChildren = hardwareChildren[index]["Children"];
    CpuParser parser(logger_, config_);
    ParseResult result = parser.parse(cpuChildren, outData);

    if (!result.success) {
        logger_.warning("CPU parse failed: " + result.errorMessage);
    }

    return result.success;
}

bool PcMetricsService::parseMemory(JsonArray hardwareChildren, int index, PcMetrics& outData) {
    if (index < 0) {
        logger_.warning("Memory section not found");
        return false;
    }

    JsonArray ramChildren = hardwareChildren[index]["Children"];
    MemoryParser parser(logger_);
    ParseResult result = parser.parse(ramChildren, outData);

    if (!result.success) {
        logger_.warning("Memory parse failed: " + result.errorMessage);
    }

    return result.success;
}

bool PcMetricsService::parseGpu(JsonArray hardwareChildren, int index, PcMetrics& outData) {
    if (index < 0) {
        logger_.warning("GPU section not found");
        return false;
    }

    JsonArray gpuChildren = hardwareChildren[index]["Children"];
    GpuParser parser(logger_);
    ParseResult result = parser.parse(gpuChildren, outData);

    if (!result.success) {
        logger_.warning("GPU parse failed: " + result.errorMessage);
    }

    return result.success;
}