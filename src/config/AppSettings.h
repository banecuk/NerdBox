#pragma once

#include <cstddef>
#include <cstdint>

#include "config/AppConfig.h"

// Connection tuning shared by every SSE stream (PcMetricsStreamJob,
// CpuClockStreamJob, ProcessStreamJob). One struct instantiated once per
// stream in AppSettings, instead of the same eight knobs spelled out as
// eight flat fields per stream — a new stream costs one AppConfig::internal
// *StreamImpl struct plus one AppSettings member line.
struct SseStreamTuning {
    uint32_t intervalMs;
    uint16_t connectTimeoutMs;
    uint16_t headerTimeoutMs;
    uint32_t reconnectBackoffMs;
    uint32_t staleTimeoutMs;
    size_t maxEventBufferBytes;
    uint16_t maxBytesPerPoll;
    const char* streamPath;

    // Built from an AppConfig::internal::*StreamImpl struct's k* constants,
    // by name — so the field order above can't silently transpose values.
    template <typename Impl>
    static constexpr SseStreamTuning from() {
        SseStreamTuning t{};
        t.intervalMs = Impl::kIntervalMs;
        t.connectTimeoutMs = Impl::kConnectTimeoutMs;
        t.headerTimeoutMs = Impl::kHeaderTimeoutMs;
        t.reconnectBackoffMs = Impl::kReconnectBackoffMs;
        t.staleTimeoutMs = Impl::kStaleTimeoutMs;
        t.maxEventBufferBytes = Impl::kMaxEventBufferBytes;
        t.maxBytesPerPoll = Impl::kMaxBytesPerPoll;
        t.streamPath = Impl::kStreamPath;
        return t;
    }
};

// Plain value object populated from AppConfig::internal's compile-time
// constants. Replaces the old AppConfigInterface/AppConfigService virtual
// dispatch pair — every value here is already known at compile time, so
// there is nothing to gain from a vtable indirection on hot paths (e.g.
// watchdogEnableOnBoot is read every 16 ms tick). Subsystems that need
// config take `const AppSettings&`.
struct AppSettings {
#define AS_FIELD(kind, type, name, init) type name = init;
#include "AppSettingsFields.def"
#undef AS_FIELD

    // Values (not sizes) — the levels a user retunes per machine. The array's
    // *size* (kBrightnessLevelCount) stays a compile-time AppConfig constant,
    // since BrightnessWidget needs it to size a fixed-length member array.
    // Not in AppSettingsFields.def: it's an array pointer, not a scalar, and
    // needs its own per-element dump in handleConfig() rather than a single
    // SEND_CONFIG_* call.
    const uint8_t* uiBrightnessLevels = AppConfig::internal::UiImpl::kBrightnessLevels;

    // Per-stream SSE connection tuning — see SseStreamTuning above. Not in
    // AppSettingsFields.def (nested struct, not a scalar); handleConfig()
    // dumps all three with one loop.
    SseStreamTuning pcMetricsStream =
        SseStreamTuning::from<AppConfig::internal::PcMetricsStreamImpl>();
    SseStreamTuning cpuClockStream =
        SseStreamTuning::from<AppConfig::internal::CpuClockStreamImpl>();
    SseStreamTuning processStream = SseStreamTuning::from<AppConfig::internal::ProcessStreamImpl>();
};
