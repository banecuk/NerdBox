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

    // Hybrid-CPU core classes. The first kThreadsPerformanceCount entries of
    // Cpu.CoreLoads are P-core threads, the rest are E-cores — NerdWinSense
    // reports them blocked P-first (confirmed on a 14700K; see
    // docs-local/13-threads-ecore-distinction.md).
    // 14700K: 8 P-cores x 2 SMT threads = 16, then 12 E-cores.
    // 0 (or >= the reported thread count) disables the distinction entirely
    // and every bar renders exactly as it does today.
    static constexpr uint8_t kThreadsPerformanceCount = 16;
    static constexpr bool kThreadsCoreClassTint = true;
};

// AirQuality configuration
struct AirQualityImpl {
    // Refresh cadence: 30 minutes, within the AirVisual free-tier rate limit.
    static constexpr uint32_t kRefreshIntervalMs = 30ul * 60ul * 1000ul;
    // Widget staleness window. The job only fetches once the data is already
    // kRefreshIntervalMs old, so a window equal to the interval blanked the
    // widget on every refresh (B42): keep it a multiple (3x).
    static constexpr uint32_t kStaleTimeoutMs = 3ul * kRefreshIntervalMs;
    // How long to wait before retrying after a failed fetch. Data is only
    // 30-minutes-fresh, so a few minutes of backoff costs nothing and avoids
    // hammering the API (and the shared background task) on outages.
    static constexpr uint32_t kFailureBackoffMs = 60000;
};

// RoomClimate (local LAN sensor) configuration
struct RoomClimateImpl {
    // Local LAN sensor: cheap to poll.
    static constexpr uint32_t kRefreshIntervalMs = 60ul * 1000ul;
    // Same 3x rule as AirQualityImpl::kStaleTimeoutMs (B42).
    static constexpr uint32_t kStaleTimeoutMs = 3ul * kRefreshIntervalMs;
    // LAN device — retry sooner than the internet-facing APIs do.
    static constexpr uint32_t kFailureBackoffMs = 15000;
};

// Weather (open-meteo forecast) configuration
struct WeatherImpl {
    // Forecast columns to render: 7 if the layout fits, otherwise drop to 5.
    static constexpr uint8_t kForecastDays = Limits::kForecastDays;
    // How often to re-fetch while the Weather screen is displayed.
    static constexpr uint32_t kRefreshIntervalMs = 2ul * 60ul * 60ul * 1000ul;  // 2 h
    // Widget staleness window. Must be a multiple of kRefreshIntervalMs:
    // WeatherJob only fetches once the data is already 1x old, so a window
    // equal to the interval blanked the forecast on every refresh (B42).
    static constexpr uint32_t kStaleTimeoutMs = 3ul * kRefreshIntervalMs;
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
    static constexpr size_t kMaxEventBufferBytes =
        1024;  // payload is tiny vs. the main stream's 4096
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
    // Sparkline takes over when CPU or GPU power draw (watts) holds at/above
    // this... Power rather than load %: short load-% spikes (app launches,
    // browser tabs) were pulling the forecast off screen too often, while
    // power only stays high under real sustained work (games, renders).
    static constexpr uint16_t kActivityEnterWatts = 50;
    // ...and hands back to the forecast only once both stay below this...
    static constexpr uint16_t kActivityExitWatts = 40;
    // ...for this long (the requested 30 s quiet period).
    static constexpr uint32_t kActivityQuietMs = 30000;
    // Power must hold this long before the sparkline takes over, so a short
    // burst never pulls the forecast off screen. The hold clock tolerates
    // dips down to kActivityExitWatts — see ActivityDetector.
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

    // WiFi auto-reconnect (NetworkManager): give up on one reconnect attempt
    // after kReconnectTimeoutMs, and start a new one at most every
    // kReconnectCheckIntervalMs.
    static constexpr uint32_t kReconnectTimeoutMs = 10000;
    static constexpr uint32_t kReconnectCheckIntervalMs = 15000;

    // Internet-reachability probes (NetworkStatusService). Steady-state
    // cadence once every endpoint has been probed; 45 s is plenty for a
    // status glyph and keeps traffic to the third-party endpoints low.
    static constexpr uint32_t kProbeIntervalMs = 45000;
    // Fast cadence right after boot, until every endpoint has one result.
    static constexpr uint32_t kWarmupProbeIntervalMs = 1000;
    // Per-probe HTTP connect/response timeout.
    static constexpr uint32_t kProbeTimeoutMs = 1500;
    // How often NetworkStatusService republishes this device's own link
    // details (A8). 1 s matches WifiLinkWidget's redraw interval, so its RSSI
    // trace still gets one fresh sample per tick.
    static constexpr uint32_t kLinkRefreshMs = 1000;
};

// NTP time sync (NtpService).
struct NtpImpl {
    // Per-attempt wait in the boot-time sync loop. Short on purpose: SNTP has
    // either resolved the time or not, and waiting longer only blocks boot.
    static constexpr uint32_t kSyncAttemptTimeoutMs = 200;
    // Spacing of the background re-sync probes while time is still unsynced.
    static constexpr uint32_t kRetryIntervalMs = 30000;
};

// Internet-facing JSON fetches (JsonHttpService: AirQuality/RoomClimate/Weather).
// Worst case per fetch is kDnsTimeoutMs + kConnectTimeoutMs + kResponseTimeoutMs
// (x kMaxAttempts), which must stay well inside WatchdogImpl::kTimeoutMs
// (20 s; static_assert below). One attempt: the calling job's failure backoff
// is the retry (B38).
struct HttpFetchImpl {
    static constexpr uint8_t kMaxAttempts = 1;
    static constexpr uint32_t kRetryDelayMs = 100;
    static constexpr uint16_t kConnectTimeoutMs = 3000;
    static constexpr uint16_t kResponseTimeoutMs = 6000;
    static constexpr uint32_t kDnsTimeoutMs = 4000;
};

// WiFi scan (WIFI screen's "nearby networks" list) configuration — see
// docs-local/13-wifi-screen-plan.md.
struct WifiScanImpl {
    // Auto-rescan cadence while the WIFI screen is open.
    static constexpr uint32_t kRescanIntervalMs = 15000;
    static constexpr uint32_t kFailureBackoffMs = 5000;
    // WiFiScanClass (esp32 Arduino core) derives its OWN internal watchdog
    // from this value — WiFiScan.cpp: `_scanTimeout = max_ms_per_chan * 20` —
    // and scanComplete() reports WIFI_SCAN_FAILED once that elapses, whether
    // or not the underlying esp_wifi scan is still genuinely in progress. A
    // scan while associated (not disconnected) has to keep hopping back to
    // the home channel to service the AP, so it runs measurably longer than
    // the naive channels*kMaxMsPerChannel estimate — confirmed on real
    // hardware: at 120 ms/channel (giving a 2400 ms internal timeout) every
    // single scan hit that timeout and reported FAILED before completing.
    // 300 ms/channel (giving a 6000 ms internal timeout — also Arduino's own
    // scanNetworks() default) leaves enough headroom for that connected-mode
    // overhead. The ~3.9 s full sweep this implies no longer trades off
    // against PcMetricsStreamImpl::kStaleTimeoutMs — none of the three SSE
    // stream jobs gate open on the WIFI screen (PcMetricsStreamJob's
    // screenGateOpen() is MAIN/GAME/DISKS only), so there's no live stream
    // for a long scan to starve.
    static constexpr uint16_t kMaxMsPerChannel = 300;
    // Our own deadline for an async scan's result, counted from the start
    // call. The core's max_ms_per_chan * 20 timeout above makes
    // scanComplete() report FAILED while the radio is still scanning, so
    // WifiScanService ignores that and waits for the real SCAN_DONE event up
    // to this long (see WifiScanMath::classifyScanPoll). Past it, the scan is
    // stopped and counted as failed.
    static constexpr uint32_t kResultTimeoutMs = 15000;
    static constexpr bool kShowHidden = true;
    // WifiLinkWidget's RSSI sparkline sample cadence.
    static constexpr uint32_t kRssiTraceIntervalMs = 1000;
};

// UI configuration
struct UiImpl {
    static constexpr uint32_t kTransitionTimeoutMs = 1000;
    // Backlight-ramp screen transition (see docs-local/03-visual-ux.md V7(a)):
    // ramp the backlight down before the outgoing screen is torn down, and
    // back up after the incoming one is built, instead of a bare
    // fillScreen(BLACK) flash. Both durations are well inside
    // kTransitionTimeoutMs above.
    static constexpr uint32_t kTransitionFadeOutMs = 80;
    static constexpr uint32_t kTransitionFadeInMs = 120;
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
    // How often DimAtNightJob checks the clock. Hour-granularity boundaries,
    // so once a minute is plenty.
    static constexpr uint32_t kDimAtNightCheckIntervalMs = 60000;
};

static_assert(HttpFetchImpl::kMaxAttempts *
                          (HttpFetchImpl::kDnsTimeoutMs + HttpFetchImpl::kConnectTimeoutMs +
                           HttpFetchImpl::kResponseTimeoutMs) +
                      (HttpFetchImpl::kMaxAttempts - 1) * HttpFetchImpl::kRetryDelayMs <
                  WatchdogImpl::kTimeoutMs,
              "a JsonHttpService fetch must finish inside the task watchdog window (B38)");

}  // namespace internal

}  // namespace AppConfig