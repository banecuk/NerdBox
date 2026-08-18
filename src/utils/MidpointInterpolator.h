#pragma once

#include <cstdint>

// Turns a step-changing signal that only arrives once per refresh cycle
// (e.g. one CPU-load sample per second) into two visible display updates per
// cycle instead of one: the average of the old and new sample right away,
// then a snap to the actual new sample halfway through the interval since
// the previous arrival. Makes a fast-moving-but-slow-refreshing metric feel
// like it updates twice as often.
//
// Pure logic, no hardware dependency — host-tested under `[env:native]`
// (test/MidpointInterpolatorTest.cpp).
class MidpointInterpolator {
 public:
    // Call once per new sample arrival (not once per redraw tick).
    void onSample(float value, uint32_t nowMs) {
        if (!hasSample_) {
            hasSample_ = true;
            prevValue_ = currentValue_ = value;
            revealAtMs_ = nowMs;
            lastArrivalMs_ = nowMs;
            revealed_ = true;
            return;
        }
        const uint32_t interval = nowMs - lastArrivalMs_;
        prevValue_ = currentValue_;
        currentValue_ = value;
        revealAtMs_ = nowMs + interval / 2;
        lastArrivalMs_ = nowMs;
        revealed_ = false;
    }

    // Value to show right now. Safe to call every redraw tick.
    float displayValue(uint32_t nowMs) const {
        return isPending(nowMs) ? (prevValue_ + currentValue_) / 2.0f : currentValue_;
    }

    // True while the midpoint average is still being shown, i.e. before the
    // halfway-through-the-cycle deadline.
    bool isPending(uint32_t nowMs) const {
        // Wrap-safe: compares via signed subtraction rather than nowMs <
        // revealAtMs_ directly, so a millis() rollover doesn't misfire.
        return static_cast<int32_t>(revealAtMs_ - nowMs) > 0;
    }

    // One-shot: true only the first call at/after the reveal deadline since
    // the last onSample() (false before the deadline, and false again on any
    // repeat call after it). Lets a caller schedule exactly one extra redraw
    // per cycle — to snap from the averaged value to the real one — instead
    // of polling every tick.
    bool consumeRevealDue(uint32_t nowMs) {
        if (revealed_ || isPending(nowMs))
            return false;
        revealed_ = true;
        return true;
    }

 private:
    bool hasSample_ = false;
    bool revealed_ = true;
    float prevValue_ = 0.0f;
    float currentValue_ = 0.0f;
    uint32_t lastArrivalMs_ = 0;
    uint32_t revealAtMs_ = 0;
};
