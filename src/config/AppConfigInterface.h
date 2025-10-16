#pragma once

#include <cstdint>

class AppConfigInterface {
public:
    virtual ~AppConfigInterface() = default;

    // Debug getters
    // virtual uint32_t getDebugSerialBaudRate() const = 0;
    // virtual uint32_t getDebugSerialTimeoutMs() const = 0;
    // virtual bool     getDebugWaitForSerial() const = 0;

    // Init getters
    virtual uint8_t  getDefaultNetworkRetries() const = 0;
    virtual uint8_t  getDefaultTimeSyncRetries() const = 0;
    virtual uint32_t getNetworkRetryDelayMs() const = 0;
    virtual uint32_t getTimeSyncRetryDelayBaseMs() const = 0;
    virtual uint16_t getBackoffJitterMs() const = 0;

    // Watchdog getters
    virtual unsigned long getWatchdogTimeoutMs() const = 0;
    virtual bool getWatchdogEnableOnBoot() const = 0;

    // Timing getters
    virtual uint32_t getTimingScreenTaskMs() const = 0;
    virtual uint32_t getTimingBackgroundTaskMs() const = 0;
    virtual uint32_t getTimingMainLoopMs() const = 0;

    // Tasks getters
    virtual uint32_t getTasksScreenStack() const = 0;
    virtual uint32_t getTasksBackgroundStack() const = 0;
    virtual uint32_t getTasksScreenPriority() const = 0;
    virtual uint32_t getTasksBackgroundPriority() const = 0;

    // HardwareMonitor getters
    // virtual uint32_t getHardwareMonitorRefreshMs() const = 0;
    // virtual uint32_t getHardwareMonitorRefreshAfterFailureMs() const = 0;
    // virtual uint32_t getHardwareMonitorRetryDelayMs() const = 0;
    // virtual uint32_t getHardwareMonitorMaxRetries() const = 0;

    // Metrics getters
    // virtual uint8_t getMaxScreenDrawTimes() const = 0;

    // PcMetrics getters
    virtual uint8_t  getPcMetricsCores() const = 0;
    
};