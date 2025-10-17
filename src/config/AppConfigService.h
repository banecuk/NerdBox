#pragma once

#include "AppConfigInterface.h"

class AppConfigService : public AppConfigInterface {
public:
    // Debug getters
    uint32_t getDebugSerialBaudRate() const override;
    uint32_t getDebugSerialTimeoutMs() const override;
    bool     getDebugWaitForSerial() const override;

    // Init getters
    uint8_t  getDefaultNetworkRetries() const override;
    uint8_t  getDefaultTimeSyncRetries() const override;
    uint32_t getNetworkRetryDelayMs() const override;
    uint32_t getTimeSyncRetryDelayBaseMs() const override;
    uint16_t getBackoffJitterMs() const override;

    // Watchdog getters
    unsigned long getWatchdogTimeoutMs() const override;
    bool getWatchdogEnableOnBoot() const override;

    // Timing getters
    uint32_t getTimingScreenTaskMs() const override;
    uint32_t getTimingBackgroundTaskMs() const override;
    uint32_t getTimingMainLoopMs() const override;

    // Tasks getters
    uint32_t getTasksScreenStack() const override;
    uint32_t getTasksBackgroundStack() const override;
    uint32_t getTasksScreenPriority() const override;
    uint32_t getTasksBackgroundPriority() const override;

    // HardwareMonitor getters
    uint32_t getHardwareMonitorRefreshMs() const override;
    uint32_t getHardwareMonitorRefreshAfterFailureMs() const override;
    uint32_t getHardwareMonitorRetryDelayMs() const override;
    uint32_t getHardwareMonitorMaxRetries() const override;

    // Metrics getters
    uint8_t getMaxScreenDrawTimes() const override;

    // PcMetrics getters
    uint8_t getPcMetricsCores() const override;

    // UI getters
    uint32_t getUiTransitionTimeoutMs() const override;
    uint32_t getUiTouchDebounceIntervalMs() const override;
    uint32_t getUiDisplayLockTimeoutMs() const override;
};