#pragma once

#include "AppConfigInterface.h"
#include "config/AppConfig.h"

class AppConfigService : public AppConfigInterface {
public:

    // Debug getters
    
    // uint32_t getDebugSerialBaudRate() const override {
    //     return Config::Debug::kSerialBaudRate;
    // }

    // uint32_t getDebugSerialTimeoutMs() const override {
    //     return Config::Debug::kSerialTimeoutMs;
    // }

    // bool getDebugWaitForSerial() const override {
    //     return Config::Debug::kWaitForSerial;
    // }

    // Init getters

    // uint8_t getDefaultNetworkRetries() const override {
    //     return Config::Init::kDefaultNetworkRetries;
    // }

    // uint8_t getDefaultTimeSyncRetries() const override {
    //     return Config::Init::kDefaultTimeSyncRetries;
    // }

    // uint32_t getNetworkRetryDelayMs() const override {
    //     return Config::Init::kNetworkRetryDelayMs;
    // }

    // uint32_t getTimeSyncRetryDelayBaseMs() const override {
    //     return Config::Init::kTimeSyncRetryDelayBaseMs;
    // }

    // uint16_t getBackoffJitterMs() const override {
    //     return Config::Init::kBackoffJitterMs;
    // }


    // Watchdog getters

    unsigned long getWatchdogTimeoutMs() const override {
        return Config::Watchdog::kTimeoutMs;
    }

    bool getWatchdogEnableOnBoot() const override {
        return Config::Watchdog::kEnableOnBoot;
    }

    // Timing getters

    uint32_t getTimingScreenTaskMs() const override {
        return Config::Timing::kScreenTaskMs;
    }

    uint32_t getTimingBackgroundTaskMs() const override {
        return Config::Timing::kBackgroundTaskMs;
    }

    uint32_t getTimingMainLoopMs() const override {
        return Config::Timing::kMainLoopMs;
    }

    // Tasks getters

    uint32_t getTasksScreenStack() const override {
        return Config::Tasks::kScreenStack;
    }

    uint32_t getTasksBackgroundStack() const override {
        return Config::Tasks::kBackgroundStack;
    }

    uint32_t getTasksScreenPriority() const override {
        return Config::Tasks::kScreenPriority;
    }

    uint32_t getTasksBackgroundPriority() const override {
        return Config::Tasks::kBackgroundPriority;
    }

    // HardwareMonitor getters

    // uint32_t getHardwareMonitorRefreshMs() const override {
    //     return HardwareMonitor::kRefreshMs;
    // }

    // uint32_t getHardwareMonitorRefreshAfterFailureMs() const override {
    //     return HardwareMonitor::kRefreshAfterFailureMs;
    // }

    // uint32_t getHardwareMonitorRetryDelayMs() const override {
    //     return HardwareMonitor::kRetryDelayMs;
    // }

    // uint32_t getHardwareMonitorMaxRetries() const override {
    //     return HardwareMonitor::kMaxRetries;
    // }

    // Metrics getters

    // uint8_t getMaxScreenDrawTimes() const override {
    //     return Metrics::kMaxScreenDrawTimes;
    // }

    // PcMetrics getters

    uint8_t getPcMetricsCores() const override {
        return Config::PcMetrics::kCores;
    }
    
};