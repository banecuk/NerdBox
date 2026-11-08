#pragma once

#include <atomic>

#include "core/ScreenTypes.h"

// Shared runtime flags. Written on one task and read on others: the screen
// task (UiController sets activeScreen), the background task on core 0 (jobs
// gate on activeScreen/isInitialized; NtpService sets isTimeSynced), and the
// web/main-loop task (/api/status). Atomic like every other cross-task flag
// in the codebase (WeatherData, WifiScanData, UiController); plain loads and
// stores through the implicit conversions are seq_cst, which is plenty here.
//
// core.isTimeSynced is the single "time synced" source: NtpService writes it
// (boot sync and background retry), and the Logger, DimAtNightJob,
// NtpRetryJob and /api/status all read it.
class SystemState {
 public:
    struct CoreState {
        std::atomic<bool> isInitialized{false};
        std::atomic<bool> isTimeSynced{false};
    };

    struct ScreenState {
        std::atomic<bool> isInitialized{false};
        std::atomic<ScreenName> activeScreen{ScreenName::NONE};
    };

    CoreState core;
    ScreenState screen;
};
