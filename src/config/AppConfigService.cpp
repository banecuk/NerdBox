#include "AppConfigService.h"
#include "config/AppConfig.h"

uint32_t AppConfigService::getDebugSerialBaudRate() const {
    return Config::Debug::kSerialBaudRate;
}

uint32_t AppConfigService::getDebugSerialTimeoutMs() const {
    return Config::Debug::kSerialTimeoutMs;
}

bool AppConfigService::getDebugWaitForSerial() const {
    return Config::Debug::kWaitForSerial;
}

uint8_t AppConfigService::getDefaultNetworkRetries() const {
    return Config::Init::kDefaultNetworkRetries;
}

uint8_t AppConfigService::getDefaultTimeSyncRetries() const {
    return Config::Init::kDefaultTimeSyncRetries;
}

uint32_t AppConfigService::getNetworkRetryDelayMs() const {
    return Config::Init::kNetworkRetryDelayMs;
}

uint32_t AppConfigService::getTimeSyncRetryDelayBaseMs() const {
    return Config::Init::kTimeSyncRetryDelayBaseMs;
}

uint16_t AppConfigService::getBackoffJitterMs() const {
    return Config::Init::kBackoffJitterMs;
}

unsigned long AppConfigService::getWatchdogTimeoutMs() const {
    return Config::Watchdog::kTimeoutMs;
}

bool AppConfigService::getWatchdogEnableOnBoot() const {
    return Config::Watchdog::kEnableOnBoot;
}

uint32_t AppConfigService::getTimingScreenTaskMs() const {
    return Config::Timing::kScreenTaskMs;
}

uint32_t AppConfigService::getTimingBackgroundTaskMs() const {
    return Config::Timing::kBackgroundTaskMs;
}

uint32_t AppConfigService::getTimingMainLoopMs() const {
    return Config::Timing::kMainLoopMs;
}

uint32_t AppConfigService::getTasksScreenStack() const {
    return Config::Tasks::kScreenStack;
}

uint32_t AppConfigService::getTasksBackgroundStack() const {
    return Config::Tasks::kBackgroundStack;
}

uint32_t AppConfigService::getTasksScreenPriority() const {
    return Config::Tasks::kScreenPriority;
}

uint32_t AppConfigService::getTasksBackgroundPriority() const {
    return Config::Tasks::kBackgroundPriority;
}

uint32_t AppConfigService::getHardwareMonitorRefreshMs() const {
    return Config::HardwareMonitor::kRefreshMs;
}

uint32_t AppConfigService::getHardwareMonitorRefreshAfterFailureMs() const {
    return Config::HardwareMonitor::kRefreshAfterFailureMs;
}

uint32_t AppConfigService::getHardwareMonitorRetryDelayMs() const {
    return Config::HardwareMonitor::kRetryDelayMs;
}

uint32_t AppConfigService::getHardwareMonitorMaxRetries() const {
    return Config::HardwareMonitor::kMaxRetries;
}

uint8_t AppConfigService::getMaxScreenDrawTimes() const {
    return Config::Metrics::kMaxScreenDrawTimes;
}

uint8_t AppConfigService::getPcMetricsCores() const {
    return Config::PcMetrics::kCores;
}

uint32_t AppConfigService::getUiTransitionTimeoutMs() const {
    return Config::Ui::kTransitionTimeoutMs;
}

uint32_t AppConfigService::getUiTouchDebounceIntervalMs() const {
    return Config::Ui::kTouchDebounceIntervalMs;
}

uint32_t AppConfigService::getUiDisplayLockTimeoutMs() const {
    return Config::Ui::kDisplayLockTimeoutMs;
}