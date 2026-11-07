#pragma once

#include <cstdint>

// Hysteresis-based "is the PC busy?" classifier backing MultiWidget's
// sparkline-vs-forecast choice (see docs-local/09-multiwidget-rotation-and-forecast-strip.md).
// Pure logic, no Arduino/hardware dependency — host-tested under
// [env:native] (test/ActivityDetectorTest.cpp).
//
// Unit-agnostic: MultiWidget feeds it CPU/GPU power draw in watts, but the
// logic only compares samples against the two levels.
//
// Two thresholds, not one: `enterLevel` is the bar for "worth interrupting the
// forecast for", `exitLevel` is the lower floor both timers key off. The enter
// hold clock starts once a sample reaches enterLevel and keeps running through
// dips as low as exitLevel — only a sample below exitLevel resets it — so an
// oscillating real workload still accumulates enterHoldMs instead of
// restarting every dip. The exit quiet clock is the mirror image: any sample
// at-or-above exitLevel resets it, so leaving active requires both metrics to
// sit below exitLevel continuously for quietMs.
class ActivityDetector {
 public:
    ActivityDetector(uint16_t enterLevel, uint16_t exitLevel, uint32_t quietMs,
                     uint32_t enterHoldMs);

    // Feed one sample. `fresh == false` means "no usable metrics" — treated
    // as below-threshold (the quiet timer keeps running, the enter hold
    // clock resets) rather than as activity.
    void tick(uint32_t nowMs, bool fresh, uint16_t cpuValue, uint16_t gpuValue);
    bool isActive() const { return active_; }

 private:
    uint16_t enterLevel_;
    uint16_t exitLevel_;
    uint32_t quietMs_;
    uint32_t enterHoldMs_;

    bool active_ = false;

    // Whether the enter-hold clock is currently accumulating, and since when
    // (only meaningful while holding_). A plain "0 = not started" sentinel on
    // the timestamp alone doesn't work — millis() legitimately is 0 at boot.
    bool holding_ = false;
    uint32_t aboveSinceMs_ = 0;

    // Same shape for the exit-quiet clock.
    bool quieting_ = false;
    uint32_t belowSinceMs_ = 0;
};
