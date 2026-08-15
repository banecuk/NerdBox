#pragma once

#include <cstddef>
#include <cstdint>

#include "config/AppConfig.h"

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
};
