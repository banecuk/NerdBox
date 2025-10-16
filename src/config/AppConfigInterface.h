#pragma once

#include <cstdint>

class AppConfigInterface {
public:
    virtual ~AppConfigInterface() = default;

    // Timing getters
    virtual uint32_t getTimingScreenTaskMs() const = 0;
    virtual uint32_t getTimingBackgroundTaskMs() const = 0;
    virtual uint32_t getTimingMainLoopMs() const = 0;

    // Tasks getters
    virtual uint32_t getTasksScreenStack() const = 0;
    virtual uint32_t getTasksBackgroundStack() const = 0;
    virtual uint32_t getTasksScreenPriority() const = 0;
    virtual uint32_t getTasksBackgroundPriority() const = 0;

    // PcMetrics getters
    virtual uint8_t  getPcMetricsCores() const = 0;
    
};