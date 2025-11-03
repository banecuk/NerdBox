#pragma once

#include <Arduino.h>

#include <array>

#include "config/AppConfigInterface.h"

class ApplicationMetrics {
 public:
    ApplicationMetrics(AppConfigInterface& config);

    // JSON parse time methods
    void setPcMetricsJsonParseTime(uint32_t timeMs);
    uint32_t getPcMetricsJsonParseTime() const;

    // Screen draw time methods
    void addScreenDrawTime(uint32_t timeMs);
    const std::vector<uint32_t>& getScreenDrawTimes() const;
    float getAverageScreenDrawTime() const;
    size_t getScreenDrawCount() const;

    // Uptime method
    String getFormattedUptime() const;

    // Thread widget FPS methods - SIMPLE FRAME COUNTER APPROACH
    void addThreadWidgetFrameTime(uint32_t timeMs);
    float getThreadWidgetFPS() const;
    size_t getThreadWidgetFrameCount() const;

 private:
    AppConfigInterface& config_;

    uint32_t pcMetricsJsonParseTime_;        // Latest JSON parse time for PC metrics
    std::vector<uint32_t> screenDrawTimes_;  // Circular buffer for screen draw times
    size_t screenDrawCapacity_;              // capacity (from config)
    size_t screenDrawIndex_;                 // Current index in the circular buffer
    size_t screenDrawCount_;                 // Number of valid entries in the buffer

    // SIMPLE FPS COUNTER - Option 3
    uint32_t threadWidgetFrameCount_ = 0;
    uint32_t threadWidgetLastFpsTime_ = 0;
    float threadWidgetCurrentFps_ = 0.0f;
};