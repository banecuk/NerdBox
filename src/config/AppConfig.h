#pragma once

#include <inttypes.h>

#include <cstddef>

namespace AppConfig {

namespace internal {
// Debug configuration
struct DebugImpl {
    static constexpr uint32_t kSerialBaudRate = 115200;
    static constexpr uint32_t kSerialTimeoutMs = 10000;
    static constexpr bool kWaitForSerial = false;
};

// Init configuration
struct InitImpl {
    static constexpr uint8_t kDefaultNetworkRetries = 10;
    static constexpr uint8_t kDefaultTimeSyncRetries = 3;
    static constexpr uint32_t kNetworkRetryDelayMs = 200;
    static constexpr uint32_t kTimeSyncRetryDelayBaseMs = 100;
    static constexpr uint16_t kBackoffJitterMs = 50;
};

// Watchdog configuration
struct WatchdogImpl {
    static constexpr unsigned long kTimeoutMs = 20000;
    static constexpr bool kEnableOnBoot = true;

    static_assert(kTimeoutMs >= 1000,
                  "kTimeoutMs is passed to esp_task_wdt_init() in whole seconds; "
                  "a value below 1000 truncates to 0");
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
    static constexpr uint32_t kBackgroundStack = 8192;
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
    static constexpr float kThreadsDownwardSmoothing = 0.075f;
};

// AirQuality configuration
struct AirQualityImpl {
    // How long to wait before retrying after a failed fetch. Data is only
    // 30-minutes-fresh, so a few minutes of backoff costs nothing and avoids
    // hammering the API (and the shared background task) on outages.
    static constexpr uint32_t kFailureBackoffMs = 60000;
};

// Weather (open-meteo forecast) configuration
struct WeatherImpl {
    // Forecast columns to render: 7 if the layout fits, otherwise drop to 5.
    static constexpr uint8_t kForecastDays = 7;
    // How often to re-fetch while the Weather screen is displayed.
    static constexpr uint32_t kRefreshIntervalMs = 2ul * 60ul * 60ul * 1000ul;  // 2 h
    // Cheap per-minute check while displayed so the forecast rolls over at
    // local midnight and shows the new day's data.
    static constexpr uint32_t kTimeCheckIntervalMs = 60ul * 1000ul;
    // Backoff after a failed fetch — mirrors AirQualityImpl::kFailureBackoffMs.
    static constexpr uint32_t kFailureBackoffMs = 60000;
    static constexpr uint8_t kIconSize = 44;
};

// Metrics configuration
struct MetricsImpl {
    static constexpr uint8_t kMaxScreenDrawTimes = 30;
};

// PcMetrics configuration
struct PcMetricsImpl {
    static constexpr uint8_t kCores = 18;
};

// PcMetrics SSE streaming configuration — see SSE-PUSH-PLAN.md.
// kEnabled defaults true as of milestone 6's cutover: confirmed on real
// hardware against the live NerdWinSense stream endpoint (chunked
// dechoding, delta-mode field parsing, reconnect-with-backoff after a
// real NerdWinSense restart, no heap growth over a multi-minute run). A
// long-duration soak and MaxStreamClients rejection behavior are still
// unverified — PcMetricsJob (polling) is deliberately kept in the tree as
// a fallback; set this back to false to revert to it without a code
// change if streaming misbehaves in the field.
struct PcMetricsStreamImpl {
    static constexpr bool kEnabled = true;
    static constexpr uint32_t kIntervalMs = 500;  // matches HardwareMonitorImpl::kRefreshMs
    static constexpr bool kDelta = true;
    static constexpr uint16_t kConnectTimeoutMs = 1000;
    static constexpr uint16_t kHeaderTimeoutMs = 2000;
    static constexpr uint32_t kReconnectBackoffMs = 2000;  // mirrors kRefreshAfterFailureMs
    // Sized for a full report (all sections + a handful of disks) with
    // headroom — not yet measured against a live payload (see plan's open
    // questions); revisit once real event sizes are known.
    static constexpr size_t kMaxEventBufferBytes = 4096;
    static constexpr uint16_t kMaxBytesPerPoll = 512;
    static constexpr const char* kStreamPath = "/api/v1/stream";
};

// UI configuration
struct UiImpl {
    static constexpr uint32_t kTransitionTimeoutMs = 1000;
    static constexpr uint32_t kTouchDebounceIntervalMs = 200;
    static constexpr uint32_t kScreenTransitionCooldownMs = 300;
    static constexpr uint32_t kDisplayLockTimeoutMs = 200;

    // NVS (Preferences) keys for persisted display settings.
    // Namespace must be <= 15 chars; key must be <= 15 chars.
    static constexpr const char* kNvsNamespace = "nerdbox_ui";
    static constexpr const char* kNvsBrightnessKey = "brightness";
    static constexpr uint8_t kDefaultBrightness = 85;

    // Six fixed brightness steps used by BrightnessWidget and cycleBrightness().
    // Ordered dim → bright. Adding or reordering levels here is the only change
    // needed — DisplayManager and BrightnessWidget both derive from this array.
    // (BrightnessWidget's per-segment labels/colors are sized off
    // kBrightnessLevelCount but still hand-listed — update those arrays too if
    // this count changes.)
    static constexpr uint8_t kBrightnessLevels[] = {20, 60, 85, 110, 140, 255};
    static constexpr uint8_t kBrightnessLevelCount = 6;

    // "Dim at night" — when enabled, brightness is reduced by kDimAtNightPercent
    // whenever the local hour is >= kDimAtNightStartHour or < kDimAtNightEndHour.
    static constexpr const char* kNvsDimAtNightKey = "dim_at_night";
    static constexpr bool kDefaultDimAtNightEnabled = false;
    static constexpr uint8_t kDimAtNightStartHour = 20;  // 20:00
    static constexpr uint8_t kDimAtNightEndHour = 6;     // 06:00
    static constexpr uint8_t kDimAtNightPercent = 60;    // reduce brightness by 60%
};
}  // namespace internal

}  // namespace AppConfig