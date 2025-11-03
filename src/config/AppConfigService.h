#pragma once

#include "AppConfig.h"
#include "AppConfigInterface.h"

class AppConfigService : public AppConfigInterface {
 public:
    // Debug getters
    uint32_t getDebugSerialBaudRate() const override {
        return AppConfig::internal::DebugImpl::kSerialBaudRate;
    }

    uint32_t getDebugSerialTimeoutMs() const override {
        return AppConfig::internal::DebugImpl::kSerialTimeoutMs;
    }

    bool getDebugWaitForSerial() const override {
        return AppConfig::internal::DebugImpl::kWaitForSerial;
    }

    // Init getters - MATCHING NAMES
    uint8_t getInitNetworkRetries() const override {
        return AppConfig::internal::InitImpl::kDefaultNetworkRetries;
    }

    uint32_t getInitNetworkRetryDelayMs() const override {
        return AppConfig::internal::InitImpl::kNetworkRetryDelayMs;
    }

    uint8_t getInitTimeSyncRetries() const override {
        return AppConfig::internal::InitImpl::kDefaultTimeSyncRetries;
    }

    uint32_t getInitTimeSyncBaseDelayMs() const override {
        return AppConfig::internal::InitImpl::kTimeSyncRetryDelayBaseMs;
    }

    uint16_t getInitBackoffJitterMs() const override {
        return AppConfig::internal::InitImpl::kBackoffJitterMs;
    }

    // Watchdog getters
    unsigned long getWatchdogTimeoutMs() const override {
        return AppConfig::internal::WatchdogImpl::kTimeoutMs;
    }

    bool getWatchdogEnableOnBoot() const override {
        return AppConfig::internal::WatchdogImpl::kEnableOnBoot;
    }

    // Timing getters
    uint32_t getTimingScreenTaskMs() const override {
        return AppConfig::internal::TimingImpl::kScreenTaskMs;
    }

    uint32_t getTimingBackgroundTaskMs() const override {
        return AppConfig::internal::TimingImpl::kBackgroundTaskMs;
    }

    uint32_t getTimingMainLoopMs() const override {
        return AppConfig::internal::TimingImpl::kMainLoopMs;
    }

    // Tasks getters
    uint32_t getTasksScreenStack() const override {
        return AppConfig::internal::TasksImpl::kScreenStack;
    }

    uint32_t getTasksBackgroundStack() const override {
        return AppConfig::internal::TasksImpl::kBackgroundStack;
    }

    uint32_t getTasksScreenPriority() const override {
        return AppConfig::internal::TasksImpl::kScreenPriority;
    }

    uint32_t getTasksBackgroundPriority() const override {
        return AppConfig::internal::TasksImpl::kBackgroundPriority;
    }

    // HardwareMonitor getters - MATCHING NAMES
    uint32_t getHardwareMonitorRefreshMs() const override {
        return AppConfig::internal::HardwareMonitorImpl::kRefreshMs;
    }

    uint32_t getHardwareMonitorThreadsRefreshMs() const override {
        return AppConfig::internal::HardwareMonitorImpl::kThreadsRefreshMs;
    }
    uint32_t getHardwareMonitorFailureRefreshMs() const override {
        return AppConfig::internal::HardwareMonitorImpl::kRefreshAfterFailureMs;
    }

    uint32_t getHardwareMonitorRetryDelayMs() const override {
        return AppConfig::internal::HardwareMonitorImpl::kRetryDelayMs;
    }

    uint32_t getHardwareMonitorMaxRetries() const override {
        return AppConfig::internal::HardwareMonitorImpl::kMaxRetries;
    }

    float getHardwareMonitorThreadsUpwardDecay() const override {
        return AppConfig::internal::HardwareMonitorImpl::kThreadsUpwardSmoothing;
    }

    float getHardwareMonitorThreadsDownwardDecay() const override {
        return AppConfig::internal::HardwareMonitorImpl::kThreadsDownwardSmoothing;
    }

    // Metrics getters - MATCHING NAMES
    uint8_t getMetricsMaxScreenDrawTimes() const override {
        return AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes;
    }

    // PcMetrics getters
    uint8_t getPcMetricsCores() const override {
        return AppConfig::internal::PcMetricsImpl::kCores;
    }

    // UI getters
    uint32_t getUiTransitionTimeoutMs() const override {
        return AppConfig::internal::UiImpl::kTransitionTimeoutMs;
    }

    uint32_t getUiTouchDebounceIntervalMs() const override {
        return AppConfig::internal::UiImpl::kTouchDebounceIntervalMs;
    }

    uint32_t getUiDisplayLockTimeoutMs() const override {
        return AppConfig::internal::UiImpl::kDisplayLockTimeoutMs;
    }
};