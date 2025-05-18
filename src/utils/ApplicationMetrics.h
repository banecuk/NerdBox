#pragma once

#include <Arduino.h>
#include <array>

#include "config/AppConfig.h"

class ApplicationMetrics {
public:
    ApplicationMetrics();

    // JSON parse time methods
    void setPcMetricsJsonParseTime(uint32_t timeMs);
    uint32_t getPcMetricsJsonParseTime() const;

    // Screen draw time methods
    void addScreenDrawTime(uint32_t timeMs);
    const std::array<uint32_t, Config::Metrics::kMaxScreenDrawTimes>& getScreenDrawTimes() const;
    float getAverageScreenDrawTime() const;
    size_t getScreenDrawCount() const; 

    // Uptime method
    String getFormattedUptime() const;

private:
    uint32_t pcMetricsJsonParseTime_; // Latest JSON parse time for PC metrics
    std::array<uint32_t, Config::Metrics::kMaxScreenDrawTimes> screenDrawTimes_; // Circular buffer for screen draw times
    size_t screenDrawIndex_; // Current index in the circular buffer
    size_t screenDrawCount_; // Number of valid entries in the buffer
};