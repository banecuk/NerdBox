#pragma once

#include "config/Limits.h"
#include "utils/PublishedFlag.h"

// Plain data struct for the opt-in per-core CPU clock SSE stream
// (GET /api/v1/stream/cpu-clock), mirroring AirQualityData/WeatherData's
// shape. Sized off kMaxThreads (48), same rationale as
// PcMetrics::cpu_thread_load[] — the reported core count is runtime data,
// not assumed to equal Limits::kCores (28).
struct CpuClockData {
    float coreClockMHz[AppConfig::Limits::kMaxThreads] = {};
    uint8_t coreCount = 0;
    float busSpeedMHz = 0.0f;
    PublishedFlag freshness;
};
