#pragma once

#include <cstdint>

// Hysteresis-based "is the PC busy?" classifier backing MultiWidget's
// sparkline-vs-forecast choice (see docs-local/09-multiwidget-rotation-and-forecast-strip.md).
// Pure logic, no Arduino/hardware dependency — host-tested under
// [env:native] (test/ActivityDetectorTest.cpp).
//
// Two thresholds, not one: `enterPct` is the bar for "worth interrupting the
// forecast for", `exitPct` is the lower floor both timers key off. The enter
// hold clock starts once a sample reaches enterPct and keeps running through
// dips as low as exitPct — only a sample below exitPct resets it — so an
// oscillating real workload (e.g. 60/90 alternating) still accumulates
// enterHoldMs instead of restarting every dip. The exit quiet clock is the
// mirror image: any sample at-or-above exitPct resets it, so leaving active
// requires both metrics to sit below exitPct continuously for quietMs.
class ActivityDetector {
 public:
    ActivityDetector(uint8_t enterPct, uint8_t exitPct, uint32_t quietMs, uint32_t enterHoldMs);

    // Feed one sample. `fresh == false` means "no usable metrics" — treated
    // as below-threshold (the quiet timer keeps running, the enter hold
    // clock resets) rather than as activity.
    void tick(uint32_t nowMs, bool fresh, uint8_t cpuLoad, uint8_t gpuLoad);
    bool isActive() const { return active_; }

 private:
    uint8_t enterPct_;
    uint8_t exitPct_;
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
