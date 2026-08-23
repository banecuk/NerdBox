#pragma once

#include <cstddef>

#include "config/Limits.h"

#include <inttypes.h>

namespace AppConfig {

namespace internal {
// Debug configuration
struct DebugImpl {
    // 115200 costs ~87 us/byte — a single 100-char log line blocks the
    // logging task for ~8.7 ms, more than half a 16 ms frame budget. Raised
    // to cut that ~8x with zero code risk. Keep in sync with
    // platformio.ini's `monitor_speed` (must match to read boot logs).
    static constexpr uint32_t kSerialBaudRate = 921600;
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

    // Staggers the *start* of each ThreadsWidget bar's move toward a new
    // target across a window after each data arrival, so 28 bars don't all
    // lurch on the same tick — see ThreadStaggerScheduler.
    static constexpr bool kThreadsStaggerEnabled = true;
    static constexpr float kThreadsStaggerFraction = 0.5f;
    static constexpr uint32_t kThreadsStaggerFallbackPeriodMs = 600;
    static constexpr uint32_t kThreadsStaggerMinPeriodMs = 100;
    static constexpr uint32_t kThreadsStaggerMaxPeriodMs = 3000;
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
    static constexpr uint8_t kForecastDays = Limits::kForecastDays;
    // How often to re-fetch while the Weather screen is displayed.
    static constexpr uint32_t kRefreshIntervalMs = 2ul * 60ul * 60ul * 1000ul;  // 2 h
    // Cheap per-minute check while displayed so the forecast rolls over at
    // local midnight and shows the new day's data.
    static constexpr uint32_t kTimeCheckIntervalMs = 60ul * 1000ul;
    // Backoff after a failed fetch — mirrors AirQualityImpl::kFailureBackoffMs.
    static constexpr uint32_t kFailureBackoffMs = 60000;
    static constexpr uint8_t kIconSize = Limits::kIconSize;
};

// Metrics configuration
struct MetricsImpl {
    static constexpr uint8_t kMaxScreenDrawTimes = Limits::kMaxScreenDrawTimes;
};

// PcMetrics configuration
struct PcMetricsImpl {
    static constexpr uint8_t kCores = Limits::kCores;
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
    // NerdWinSense sends an event every ~kIntervalMs regardless of whether
    // any section actually changed (handleEvent() stamps lastEventMs_ before
    // checking for changed sections), so a healthy connection's event gap
    // never approaches this. Guards against a half-open TCP connection:
    // WiFiClient::connected() can keep reporting true after the peer is
    // gone, so SseConnection::poll() alone may never notice the drop.
    static constexpr uint32_t kStaleTimeoutMs = 5000;
    // Sized for a full report (all sections + a handful of disks) with
    // headroom — not yet measured against a live payload (see plan's open
    // questions); revisit once real event sizes are known.
    static constexpr size_t kMaxEventBufferBytes = 4096;
    // 2048 so a full ~1.6 KB SSE event drains in a single background tick
    // instead of 3-4 (measured against a live NerdWinSense payload).
    static constexpr uint16_t kMaxBytesPerPoll = 2048;
    static constexpr const char* kStreamPath = "/api/v1/stream";
};

// CPU-clock SSE streaming configuration — see docs-local/CPU-CLOCK-SCREEN-PLAN.md.
// Fetched only while the CPU_CLOCK screen is active (opt-in, single-metric
// endpoint) — no polling fallback and no delta mode, so there's nothing to
// toggle here beyond the connection tuning itself.
struct CpuClockStreamImpl {
    static constexpr uint32_t kIntervalMs = 1000;
    static constexpr uint16_t kConnectTimeoutMs = 1000;
    static constexpr uint16_t kHeaderTimeoutMs = 2000;
    static constexpr uint32_t kReconnectBackoffMs = 2000;
    static constexpr uint32_t kStaleTimeoutMs = 5000;
    static constexpr size_t kMaxEventBufferBytes = 1024;  // payload is tiny vs. the main stream's 4096
    static constexpr uint16_t kMaxBytesPerPoll = 1024;
    static constexpr const char* kStreamPath = "/api/v1/stream/cpu-clock";
};

// Process-list SSE streaming configuration — see
// docs-local/PROCESSES-SCREEN-PLAN.md. Fetched only while the PROCESSES
// screen is active (opt-in, no delta mode) — same rationale as
// CpuClockStreamImpl.
struct ProcessStreamImpl {
    static constexpr uint32_t kIntervalMs = 2000;
    static constexpr uint16_t kConnectTimeoutMs = 1000;
    static constexpr uint16_t kHeaderTimeoutMs = 2000;
    static constexpr uint32_t kReconnectBackoffMs = 2000;
    static constexpr uint32_t kStaleTimeoutMs = 6000;
    static constexpr size_t kMaxEventBufferBytes = 3072;
    static constexpr uint16_t kMaxBytesPerPoll = 2048;
    static constexpr const char* kStreamPath = "/api/v1/stream/processes";
};

// Audio (MusicBee mb_NerdBox plugin push) configuration — see
// docs-local/NERDBOX_INTEGRATION.md.
struct AudioImpl {
    // How long MultiWidget keeps showing the "Stopped"/"Disconnected"
    // message after a stop/offline event before falling back to the
    // sparkline widget, if no new track has started in the meantime.
    static constexpr uint32_t kStoppedMessageMs = 2000;
    // How long MultiWidget keeps showing a paused track before falling back
    // to the sparkline widget, if playback hasn't resumed in the meantime.
    static constexpr uint32_t kPausedTimeoutMs = 10000;
};

// MultiWidget configuration — sparkline/forecast/audio priority rotation.
// See docs-local/09-multiwidget-rotation-and-forecast-strip.md.
struct MultiWidgetImpl {
    // Sparkline takes over when CPU or GPU load holds at/above this...
    static constexpr uint8_t kActivityEnterPct = 70;
    // ...and hands back to the forecast only once both stay below this...
    static constexpr uint8_t kActivityExitPct = 50;
    // ...for this long (the requested 30 s quiet period).
    static constexpr uint32_t kActivityQuietMs = 30000;
    // Load must hold this long before the sparkline takes over, so a short
    // burst (anything under 5 s) never pulls the forecast off screen. The
    // hold clock tolerates dips down to kActivityExitPct — see
    // ActivityDetector.
    static constexpr uint32_t kActivityEnterHoldMs = 6000;
    // Forecast columns in the 82px strip — fewer than Limits::kForecastDays
    // on purpose; the full 7-day view lives on the weather screen.
    static constexpr uint8_t kForecastDays = 6;
};

// Network configuration
struct NetworkImpl {
    // Advertised via mDNS so the device is reachable as
    // http://<kMdnsHostname>.local instead of a DHCP-assigned IP. Must be
    // <= 63 chars and contain no dots (mDNS hostname rules).
    static constexpr const char* kMdnsHostname = "nerdbox";
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
    static constexpr uint8_t kBrightnessLevelCount = Limits::kBrightnessLevelCount;

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