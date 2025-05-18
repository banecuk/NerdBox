#pragma once

#include <inttypes.h>
#include <freertos/FreeRTOS.h>


namespace Config {

namespace Debug {
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kSerialTimeoutMs = 10000;
constexpr bool kWaitForSerial = false;
};

namespace Init {
constexpr uint8_t kDefaultNetworkRetries = 3;
constexpr uint8_t kDefaultTimeSyncRetries = 3;
constexpr uint32_t kNetworkRetryDelayMs = 200;
constexpr uint32_t kTimeSyncRetryDelayBaseMs = 100;

constexpr uint16_t kBackoffJitterMs = 50;
};

namespace Watchdog {
constexpr unsigned long kTimeoutMs = 20000;  // 20s timeout, TODO reduce
constexpr bool kEnableOnBoot = true;
} 

namespace Timing {
constexpr uint32_t kScreenTaskMs = 33;
constexpr uint32_t kBackgroundTaskMs = 20;
constexpr uint32_t kMainLoopMs = 10;
};

namespace Tasks {
constexpr uint32_t kScreenStack = 4096;
constexpr uint32_t kBackgroundStack = 8096;
constexpr UBaseType_t kScreenPriority = 2;
constexpr UBaseType_t kBackgroundPriority = 1;
};

namespace HardwareMonitor {
constexpr uint32_t kRefreshMs = 2000;
constexpr uint32_t kRefreshAfterFailureMs = 3000;
constexpr uint32_t kRetryDelayMs = 200;
constexpr uint32_t kMaxRetries = 2;
};

namespace Metrics {
constexpr uint8_t kMaxScreenDrawTimes = 30;
};

}  // namespace Config