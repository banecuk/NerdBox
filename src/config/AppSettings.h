#pragma once

#include <cstddef>
#include <cstdint>

#include "AppConfig.h"

// Plain value object populated from AppConfig::internal's compile-time
// constants. Replaces the old AppConfigInterface/AppConfigService virtual
// dispatch pair — every value here is already known at compile time, so
// there is nothing to gain from a vtable indirection on hot paths (e.g.
// watchdogEnableOnBoot is read every 16 ms tick). Subsystems that need
// config take `const AppSettings&`.
struct AppSettings {
    // Debug
    uint32_t debugSerialBaudRate = AppConfig::internal::DebugImpl::kSerialBaudRate;
    uint32_t debugSerialTimeoutMs = AppConfig::internal::DebugImpl::kSerialTimeoutMs;
    bool debugWaitForSerial = AppConfig::internal::DebugImpl::kWaitForSerial;

    // Init
    uint8_t initNetworkRetries = AppConfig::internal::InitImpl::kDefaultNetworkRetries;
    uint32_t initNetworkRetryDelayMs = AppConfig::internal::InitImpl::kNetworkRetryDelayMs;
    uint8_t initTimeSyncRetries = AppConfig::internal::InitImpl::kDefaultTimeSyncRetries;
    uint32_t initTimeSyncBaseDelayMs = AppConfig::internal::InitImpl::kTimeSyncRetryDelayBaseMs;
    uint16_t initBackoffJitterMs = AppConfig::internal::InitImpl::kBackoffJitterMs;

    // Watchdog
    unsigned long watchdogTimeoutMs = AppConfig::internal::WatchdogImpl::kTimeoutMs;
    bool watchdogEnableOnBoot = AppConfig::internal::WatchdogImpl::kEnableOnBoot;

    // Timing
    uint32_t timingScreenTaskMs = AppConfig::internal::TimingImpl::kScreenTaskMs;
    uint32_t timingBackgroundTaskMs = AppConfig::internal::TimingImpl::kBackgroundTaskMs;
    uint32_t timingMainLoopMs = AppConfig::internal::TimingImpl::kMainLoopMs;

    // Tasks
    uint32_t tasksScreenStack = AppConfig::internal::TasksImpl::kScreenStack;
    uint32_t tasksBackgroundStack = AppConfig::internal::TasksImpl::kBackgroundStack;
    uint32_t tasksScreenPriority = AppConfig::internal::TasksImpl::kScreenPriority;
    uint32_t tasksBackgroundPriority = AppConfig::internal::TasksImpl::kBackgroundPriority;

    // HardwareMonitor
    uint32_t hardwareMonitorRefreshMs = AppConfig::internal::HardwareMonitorImpl::kRefreshMs;
    uint32_t hardwareMonitorThreadsRefreshMs =
        AppConfig::internal::HardwareMonitorImpl::kThreadsRefreshMs;
    uint32_t hardwareMonitorFailureRefreshMs =
        AppConfig::internal::HardwareMonitorImpl::kRefreshAfterFailureMs;
    uint32_t hardwareMonitorRetryDelayMs = AppConfig::internal::HardwareMonitorImpl::kRetryDelayMs;
    uint32_t hardwareMonitorMaxRetries = AppConfig::internal::HardwareMonitorImpl::kMaxRetries;
    float hardwareMonitorThreadsUpwardSmoothing =
        AppConfig::internal::HardwareMonitorImpl::kThreadsUpwardSmoothing;
    float hardwareMonitorThreadsDownwardSmoothing =
        AppConfig::internal::HardwareMonitorImpl::kThreadsDownwardSmoothing;

    // AirQuality
    uint32_t airQualityFailureBackoffMs = AppConfig::internal::AirQualityImpl::kFailureBackoffMs;

    // Weather
    uint32_t weatherRefreshIntervalMs = AppConfig::internal::WeatherImpl::kRefreshIntervalMs;
    uint32_t weatherTimeCheckIntervalMs = AppConfig::internal::WeatherImpl::kTimeCheckIntervalMs;
    uint32_t weatherFailureBackoffMs = AppConfig::internal::WeatherImpl::kFailureBackoffMs;
    uint8_t weatherForecastDays = AppConfig::internal::WeatherImpl::kForecastDays;

    // Metrics
    uint8_t metricsMaxScreenDrawTimes = AppConfig::internal::MetricsImpl::kMaxScreenDrawTimes;

    // PcMetrics
    uint8_t pcMetricsCores = AppConfig::internal::PcMetricsImpl::kCores;

    // PcMetrics streaming (SSE) — see SSE-PUSH-PLAN.md
    bool pcMetricsStreamEnabled = AppConfig::internal::PcMetricsStreamImpl::kEnabled;
    uint32_t pcMetricsStreamIntervalMs = AppConfig::internal::PcMetricsStreamImpl::kIntervalMs;
    bool pcMetricsStreamDelta = AppConfig::internal::PcMetricsStreamImpl::kDelta;
    uint16_t pcMetricsStreamConnectTimeoutMs =
        AppConfig::internal::PcMetricsStreamImpl::kConnectTimeoutMs;
    uint16_t pcMetricsStreamHeaderTimeoutMs =
        AppConfig::internal::PcMetricsStreamImpl::kHeaderTimeoutMs;
    uint32_t pcMetricsStreamReconnectBackoffMs =
        AppConfig::internal::PcMetricsStreamImpl::kReconnectBackoffMs;
    uint32_t pcMetricsStreamStaleTimeoutMs =
        AppConfig::internal::PcMetricsStreamImpl::kStaleTimeoutMs;
    size_t pcMetricsStreamMaxEventBufferBytes =
        AppConfig::internal::PcMetricsStreamImpl::kMaxEventBufferBytes;
    uint16_t pcMetricsStreamMaxBytesPerPoll =
        AppConfig::internal::PcMetricsStreamImpl::kMaxBytesPerPoll;
    const char* pcMetricsStreamPath = AppConfig::internal::PcMetricsStreamImpl::kStreamPath;

    // Network
    const char* networkMdnsHostname = AppConfig::internal::NetworkImpl::kMdnsHostname;

    // UI
    uint32_t uiTransitionTimeoutMs = AppConfig::internal::UiImpl::kTransitionTimeoutMs;
    uint32_t uiTouchDebounceIntervalMs = AppConfig::internal::UiImpl::kTouchDebounceIntervalMs;
    uint32_t uiScreenTransitionCooldownMs =
        AppConfig::internal::UiImpl::kScreenTransitionCooldownMs;
    uint32_t uiDisplayLockTimeoutMs = AppConfig::internal::UiImpl::kDisplayLockTimeoutMs;
    uint8_t uiDimAtNightStartHour = AppConfig::internal::UiImpl::kDimAtNightStartHour;
    uint8_t uiDimAtNightEndHour = AppConfig::internal::UiImpl::kDimAtNightEndHour;
    uint8_t uiDimAtNightPercent = AppConfig::internal::UiImpl::kDimAtNightPercent;
    bool uiDefaultDimAtNightEnabled = AppConfig::internal::UiImpl::kDefaultDimAtNightEnabled;
    const char* uiNvsNamespace = AppConfig::internal::UiImpl::kNvsNamespace;
    const char* uiNvsBrightnessKey = AppConfig::internal::UiImpl::kNvsBrightnessKey;
    const char* uiNvsDimAtNightKey = AppConfig::internal::UiImpl::kNvsDimAtNightKey;
    uint8_t uiDefaultBrightness = AppConfig::internal::UiImpl::kDefaultBrightness;
    // Values (not sizes) — the levels a user retunes per machine. The array's
    // *size* (kBrightnessLevelCount) stays a compile-time AppConfig constant,
    // since BrightnessWidget needs it to size a fixed-length member array.
    const uint8_t* uiBrightnessLevels = AppConfig::internal::UiImpl::kBrightnessLevels;
};
