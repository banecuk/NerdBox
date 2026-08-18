#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "config/Limits.h"

// Staggers the *start* of each ThreadsWidget bar's move toward a new target
// across a window after each data arrival, so 28 bars don't all lurch on the
// same tick. Pure logic, no LGFX/Arduino/FreeRTOS dependency — host-testable
// under [env:native], same rationale as MetricColorPolicy.
//
// An "epoch" is an observed change in the data publisher's timestamp
// (PcMetrics::freshness.lastUpdateMs()). On each epoch the measured period
// (EMA of inter-epoch deltas, clamped) sets a release window
// (windowFraction * period), and every bar is assigned a release offset
// inside that window according to one of kPatternCount rotating patterns —
// never the same pattern twice in a row.
class ThreadStaggerScheduler {
 public:
    static constexpr uint8_t kPatternCount = 6;

    void configure(uint8_t barCount, float windowFraction, uint32_t fallbackPeriodMs,
                   uint32_t minPeriodMs, uint32_t maxPeriodMs);
    void seed(uint32_t seedValue);

    // Call once per tick with the current clock and the publisher's
    // timestamp; detects epochs (a change in publishStampMs) itself and
    // no-ops otherwise.
    void tick(uint32_t nowMs, uint32_t publishStampMs);

    // Releases every bar immediately (window = 0) — boot, stale->fresh
    // recovery, or the feature disabled.
    void releaseAll(uint32_t nowMs);

    bool isReleased(uint8_t index, uint32_t nowMs) const;

    // Introspection, for tests and (optionally) /app-info.
    uint32_t periodMs() const;
    uint32_t windowMs() const { return windowMs_; }
    uint8_t lastPattern() const { return lastPattern_; }

 private:
    void beginEpoch(uint32_t nowMs);
    void assignRanks();
    uint32_t nextRandom();

    uint8_t barCount_ = 0;
    float windowFraction_ = 0.5f;
    uint32_t fallbackPeriodMs_ = 600;
    uint32_t minPeriodMs_ = 100;
    uint32_t maxPeriodMs_ = 3000;

    bool hasStamp_ = false;
    bool hasPeriod_ = false;
    uint32_t lastStamp_ = 0;
    uint32_t lastEpochMs_ = 0;
    uint32_t periodMs_ = 0;
    uint32_t windowMs_ = 0;

    uint32_t rngState_ = 1;
    // kPatternCount is not a valid pattern index, so the very first
    // assignRanks() call never triggers the "don't repeat" bump.
    uint8_t lastPattern_ = kPatternCount;

    std::vector<uint8_t> ranks_;
    std::vector<uint32_t> releaseAtMs_;

    // Scratch buffers for assignRanks()'s CenterOut/EdgesIn/Shuffle patterns.
    // Fixed-size (barCount_ is capped by AppConfig::Limits::kMaxThreads via
    // the static_assert in Limits.h) so assignRanks() never heap-allocates —
    // it runs once per epoch (~2/s) on the screen task.
    std::array<uint8_t, AppConfig::Limits::kMaxThreads> order_{};
    std::array<uint8_t, AppConfig::Limits::kMaxThreads> perm_{};
};
