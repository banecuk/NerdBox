#pragma once

#include <ArduinoJson.h>

#include <cstdlib>

#include "config/AppConfigInterface.h"
#include "SensorFinder.h"
#include "services/pcMetrics/PcMetrics.h"
#include "utils/LoggerInterface.h"

/**
 * Result of parsing operation
 */
struct ParseResult {
    bool success;
    String errorMessage;

    static ParseResult Ok() { return {true, ""}; }
    static ParseResult Err(const String& msg) { return {false, msg}; }
};

/**
 * Base class for hardware component parsers
 */
class HardwareParser {
 public:
    HardwareParser(LoggerInterface& logger) : logger_(logger) {}
    virtual ~HardwareParser() = default;

    virtual ParseResult parse(JsonArray children, PcMetrics& out) = 0;

 protected:
    LoggerInterface& logger_;

    /**
     * Parse a numeric value from JsonVariant with type conversion
     */
    template <typename T>
    T parseValue(JsonVariant value, T defaultValue) {
        if (value.isNull()) {
            return defaultValue;
        }
        if (value.is<float>() || value.is<int>()) {
            float val = value.as<float>();
            if constexpr (std::is_integral_v<T>) {
                return static_cast<T>(val + 0.5f);  // Round to nearest
            }
            return static_cast<T>(val);
        }
        // Handle string values (e.g., "45.0 %", "40.5 °C")
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
            return static_cast<T>(result + 0.5f);  // Round to nearest
        }
        return static_cast<T>(result);
    }
};

/**
 * Parser for CPU metrics (load, power, temperatures)
 */
class CpuParser : public HardwareParser {
 public:
    CpuParser(LoggerInterface& logger, AppConfigInterface& config)
        : HardwareParser(logger), config_(config) {}

    ParseResult parse(JsonArray children, PcMetrics& out) override;

 private:
    AppConfigInterface& config_;
    static constexpr size_t CPU_LOAD_OFFSET = 2;  // Skip CPU total and core max

    ParseResult parseLoadMetrics(JsonArray loadSensors, PcMetrics& out);
    ParseResult parsePowerMetrics(JsonArray powerSensors, PcMetrics& out);
};

/**
 * Parser for GPU metrics (load, memory, temperatures)
 */
class GpuParser : public HardwareParser {
 public:
    GpuParser(LoggerInterface& logger) : HardwareParser(logger) {}

    ParseResult parse(JsonArray children, PcMetrics& out) override;

 private:
    static constexpr float GPU_MEMORY_CAPACITY_MB = 16368.0f;

    ParseResult parseLoadMetrics(JsonArray loadSensors, PcMetrics& out);
    ParseResult parseMemoryMetrics(JsonArray dataSensors, PcMetrics& out);
};

/**
 * Parser for motherboard metrics (temperatures, fans)
 */
class MotherboardParser : public HardwareParser {
 public:
    MotherboardParser(LoggerInterface& logger) : HardwareParser(logger) {}

    ParseResult parse(JsonArray children, PcMetrics& out) override;

 private:
    ParseResult parseTemperatures(JsonArray tempSensors, PcMetrics& out);
    ParseResult parseFans(JsonArray fanSensors, PcMetrics& out);
};

/**
 * Parser for memory/RAM metrics
 */
class MemoryParser : public HardwareParser {
 public:
    MemoryParser(LoggerInterface& logger) : HardwareParser(logger) {}

    ParseResult parse(JsonArray children, PcMetrics& out) override;
};