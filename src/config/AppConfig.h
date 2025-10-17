#pragma once

#include <inttypes.h>

namespace Config {

struct Debug {
    static constexpr uint32_t kSerialBaudRate = 115200;
    static constexpr uint32_t kSerialTimeoutMs = 10000;
    static constexpr bool kWaitForSerial = false;
};

struct Init {
    static constexpr uint8_t kDefaultNetworkRetries = 3;
    static constexpr uint8_t kDefaultTimeSyncRetries = 3;
    static constexpr uint32_t kNetworkRetryDelayMs = 200;
    static constexpr uint32_t kTimeSyncRetryDelayBaseMs = 100;
    static constexpr uint16_t kBackoffJitterMs = 50;
};

struct Watchdog {
    static constexpr unsigned long kTimeoutMs = 20000;  // 20s timeout, TODO reduce
    static constexpr bool kEnableOnBoot = true;
};

struct Timing {
    static constexpr uint32_t kScreenTaskMs = 33;
    static constexpr uint32_t kBackgroundTaskMs = 20;
    static constexpr uint32_t kMainLoopMs = 10;
};

struct Tasks {
    static constexpr uint32_t kScreenStack = 4096;
    static constexpr uint32_t kBackgroundStack = 8096;
    static constexpr uint32_t kScreenPriority = 2;
    static constexpr uint32_t kBackgroundPriority = 1;
};

struct HardwareMonitor {
    static constexpr uint32_t kRefreshMs = 500;
    static constexpr uint32_t kRefreshAfterFailureMs = 3000;
    static constexpr uint32_t kRetryDelayMs = 200;
    static constexpr uint32_t kMaxRetries = 2;
};

struct Metrics {
    static constexpr uint8_t kMaxScreenDrawTimes = 30;
};

struct PcMetrics {
    static constexpr uint8_t kCores = 18;
};

struct Ui {
    static constexpr uint32_t kTransitionTimeoutMs = 1000;
    static constexpr uint32_t kTouchDebounceIntervalMs = 200;
    static constexpr uint32_t kDisplayLockTimeoutMs = 200;
};

}  // namespace Config