#pragma once

#include <inttypes.h>

namespace AppConfig {

// Public empty namespaces - direct usage will cause linker errors
namespace Debug {};
namespace Init {};
namespace Watchdog {};
namespace Timing {};
namespace Tasks {};
namespace HardwareMonitor {};
namespace Metrics {};
namespace PcMetrics {};
namespace Ui {};

// Private implementation details
namespace internal {
// Debug configuration
struct DebugImpl {
    static constexpr uint32_t kSerialBaudRate = 115200;
    static constexpr uint32_t kSerialTimeoutMs = 10000;
    static constexpr bool kWaitForSerial = false;
};

// Init configuration
struct InitImpl {
    static constexpr uint8_t kDefaultNetworkRetries = 3;
    static constexpr uint8_t kDefaultTimeSyncRetries = 3;
    static constexpr uint32_t kNetworkRetryDelayMs = 200;
    static constexpr uint32_t kTimeSyncRetryDelayBaseMs = 100;
    static constexpr uint16_t kBackoffJitterMs = 50;
};

// Watchdog configuration
struct WatchdogImpl {
    static constexpr unsigned long kTimeoutMs = 20000;
    static constexpr bool kEnableOnBoot = true;
};

// Timing configuration
struct TimingImpl {
    static constexpr uint32_t kScreenTaskMs = 16;
    static constexpr uint32_t kBackgroundTaskMs = 20;
    static constexpr uint32_t kMainLoopMs = 10;
};

// Tasks configuration
struct TasksImpl {
    static constexpr uint32_t kScreenStack = 6144;
    static constexpr uint32_t kBackgroundStack = 8096;
    static constexpr uint32_t kScreenPriority = 2;
    static constexpr uint32_t kBackgroundPriority = 1;
};

// HardwareMonitor configuration
struct HardwareMonitorImpl {
    static constexpr uint32_t kRefreshMs = 500;
    static constexpr uint32_t kThreadsRefreshMs = 16;
    static constexpr uint32_t kRefreshAfterFailureMs = 3000;
    static constexpr uint32_t kRetryDelayMs = 200;
    static constexpr uint32_t kMaxRetries = 2;
    static constexpr float kThreadsUpwardSmoothing = 0.4f;
    static constexpr float kThreadsDownwardSmoothing = 0.05f;
};

// Metrics configuration
struct MetricsImpl {
    static constexpr uint8_t kMaxScreenDrawTimes = 30;
};

// PcMetrics configuration
struct PcMetricsImpl {
    static constexpr uint8_t kCores = 18;
};

// UI configuration
struct UiImpl {
    static constexpr uint32_t kTransitionTimeoutMs = 1000;
    static constexpr uint32_t kTouchDebounceIntervalMs = 200;
    static constexpr uint32_t kDisplayLockTimeoutMs = 200;

    // NVS (Preferences) keys for persisted display settings.
    // Namespace must be <= 15 chars; key must be <= 15 chars.
    static constexpr const char* kNvsNamespace = "nerdbox_ui";
    static constexpr const char* kNvsBrightnessKey = "brightness";
    static constexpr uint8_t kDefaultBrightness = 75;

    // Five fixed brightness steps used by BrightnessWidget and cycleBrightness().
    // Ordered dim → bright. Adding or reordering levels here is the only change
    // needed — DisplayManager and BrightnessWidget both derive from this array.
    static constexpr uint8_t kBrightnessLevels[] = {20, 50, 75, 140, 255};
    static constexpr uint8_t kBrightnessLevelCount = 5;
};
}  // namespace internal

}  // namespace AppConfig