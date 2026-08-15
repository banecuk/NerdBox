#pragma once

#include <inttypes.h>

// Compile-time capacities that are genuinely public: array sizes and loop
// bounds needed by code outside config/ (widget member arrays, parser loop
// bounds, struct field sizes). These cannot live behind AppSettings — a
// runtime value struct can't size a fixed-length array — so CLAUDE.md's "read
// AppConfig::internal only from AppSettings's field initializers" rule
// doesn't apply here; it's scoped to tuning values, not these.
//
// AppConfig::internal derives its own copies of these constants (so tuning
// structs stay self-contained reading-wise); this header is the single
// source of truth both sides agree with.
namespace AppConfig {
namespace Limits {

// Weather forecast columns (WeatherWidget, WeatherData, WeatherService).
static constexpr uint8_t kForecastDays = 7;
static constexpr uint8_t kIconSize = 44;

// Brightness segment count (DisplayManager, BrightnessWidget).
static constexpr uint8_t kBrightnessLevelCount = 6;

// ApplicationMetrics draw-time ring capacity (ServiceBundle's cross-check).
static constexpr uint8_t kMaxScreenDrawTimes = 30;

// Number of rotating probe endpoints NetworkStatusService cycles through for
// internet-reachability checks (NetworkStatus::endpoint_ok[], WebApiHandlers'
// /api/status "probes" array). Single source of truth so adding/removing a
// probe URL can't leave endpoint_ok[] under/over-sized relative to the count
// that actually gets written.
static constexpr uint8_t kNetworkProbeEndpoints = 6;

// Logical CPU thread count this build expects NerdWinSense to report.
static constexpr uint8_t kCores = 28;

// PcMetrics::cpu_thread_load[] capacity — must stay >= kCores or a PC
// reporting kCores threads walks past the array with no diagnostic.
static constexpr uint8_t kMaxThreads = 48;

static_assert(kCores <= kMaxThreads,
              "kCores exceeds PcMetrics::cpu_thread_load capacity (kMaxThreads)");

}  // namespace Limits
}  // namespace AppConfig
