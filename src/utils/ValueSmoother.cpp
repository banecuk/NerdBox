#include "ValueSmoother.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace {
// A NaN smoothing factor would otherwise poison every subsequent value
// forever (prev*(1-a) + new*a stays NaN); an out-of-range one would let a
// single bad caller destabilize the EMA. Clamp instead of trusting the caller.
float sanitizeSmoothing(float value) {
    if (std::isnan(value)) {
        return 1.0f;
    }
    return std::clamp(value, 0.0f, 1.0f);
}
}  // namespace

ValueSmoother::ValueSmoother(size_t size, float upwardSmoothing, float downwardSmoothing)
    : size_(std::max<size_t>(size, 1)),
      upwardSmoothing_(sanitizeSmoothing(upwardSmoothing)),
      downwardSmoothing_(sanitizeSmoothing(downwardSmoothing)),
      smoothedValues_(size_, 0.0f) {}

void ValueSmoother::update(const uint8_t* newValues, size_t count) {
    assert(newValues != nullptr && "Input array cannot be null");

    if (!hasPrevious_) {
        // First update - initialize with raw values
        const size_t processCount = std::min(size_, count);
        for (size_t i = 0; i < processCount; ++i) {
            smoothedValues_[i] = static_cast<float>(newValues[i]);
        }
        hasPrevious_ = true;
        return;
    }

    const size_t processCount = std::min(size_, count);
    for (size_t i = 0; i < processCount; ++i) {
        const float newValue = static_cast<float>(newValues[i]);
        const float previousValue = smoothedValues_[i];

        // Choose smoothing factor based on value direction
        const float smoothing = (newValue > previousValue) ? upwardSmoothing_ : downwardSmoothing_;

        // Apply exponential smoothing
        smoothedValues_[i] = previousValue * (1.0f - smoothing) + newValue * smoothing;
    }
}

void ValueSmoother::update(const std::vector<uint8_t>& newValues) {
    update(newValues.data(), newValues.size());
}

uint8_t ValueSmoother::getSmoothedValue(size_t index) const {
    // ESP32 Arduino builds ship with asserts enabled even in release, so a
    // bad index here must not abort — an assert would just reboot the
    // device. Clamp to a safe default instead of relying on callers.
    if (index >= size_) {
        return 0;
    }
    return static_cast<uint8_t>(smoothedValues_[index] + ROUNDING_OFFSET);
}

void ValueSmoother::getSmoothedValues(uint8_t* output, size_t count) const {
    assert(output != nullptr && "Output array cannot be null");
    const size_t copyCount = std::min(size_, count);
    for (size_t i = 0; i < copyCount; ++i) {
        output[i] = static_cast<uint8_t>(smoothedValues_[i] + ROUNDING_OFFSET);
    }
}

void ValueSmoother::getSmoothedValues(std::vector<uint8_t>& output) const {
    output.resize(size_);
    getSmoothedValues(output.data(), output.size());
}

void ValueSmoother::reset() {
    hasPrevious_ = false;
    std::fill(smoothedValues_.begin(), smoothedValues_.end(), 0.0f);
}