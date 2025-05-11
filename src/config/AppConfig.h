#pragma once

#include <inttypes.h>

namespace Config {

namespace Debug {
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kSerialTimeoutMs = 10000;
constexpr bool kWaitForSerial = false;
};  // namespace Debug

namespace Init {
constexpr uint8_t kDefaultNetworkRetries = 3;
constexpr uint8_t kDefaultTimeSyncRetries = 3;
constexpr uint32_t kNetworkRetryDelayMs = 200;
constexpr uint32_t kTimeSyncRetryDelayBaseMs = 100;

constexpr uint16_t kBackoffJitterMs = 50;
};  // namespace Init

namespace Watchdog {
constexpr unsigned long kTimeoutMs = 30000;  // 30s timeout, TODO reduce
constexpr bool kEnableOnBoot = true;
}  // namespace Watchdog

namespace Timing {
constexpr uint32_t kScreenTaskMs = 50;
constexpr uint32_t kBackgroundTaskMs = 20;
constexpr uint32_t kMainLoopMs = 10;
};  // namespace Timing

namespace Tasks {
constexpr uint32_t kScreenStack = 10000;
constexpr uint32_t kBackgroundStack = 20000;

};  // namespace Tasks

namespace HardwareMonitor {
constexpr uint32_t kRefreshMs = 2000;
constexpr uint32_t kRefreshAfterFailureMs = 3000;
constexpr uint32_t kRetryDelayMs = 200;
constexpr uint32_t kMaxRetries = 2;
};  // namespace HardwareMonitor

}  // namespace Config