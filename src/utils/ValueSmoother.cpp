#include "ValueSmoother.h"

#include <algorithm>
#include <cassert>

ValueSmoother::ValueSmoother(size_t size, float upwardSmoothing, float downwardSmoothing)
    : size_(size),
      upwardSmoothing_(upwardSmoothing),
      downwardSmoothing_(downwardSmoothing),
      smoothedValues_(size, 0.0f),
      previousValues_(size, 0.0f) {
    assert(size > 0 && "Size must be greater than zero");
    assert(upwardSmoothing >= 0.0f && upwardSmoothing <= 1.0f &&
           "Upward smoothing must be in range [0, 1]");
    assert(downwardSmoothing >= 0.0f && downwardSmoothing <= 1.0f &&
           "Downward smoothing must be in range [0, 1]");
}

void ValueSmoother::update(const uint8_t* newValues, size_t count) {
    assert(newValues != nullptr && "Input array cannot be null");

    if (!hasPrevious_) {
        // First update - initialize with raw values
        const size_t processCount = std::min(size_, count);
        for (size_t i = 0; i < processCount; ++i) {
            const float rawValue = static_cast<float>(newValues[i]);
            smoothedValues_[i] = rawValue;
            previousValues_[i] = rawValue;
        }
        hasPrevious_ = true;
        return;
    }

    const size_t processCount = std::min(size_, count);
    for (size_t i = 0; i < processCount; ++i) {
        const float newValue = static_cast<float>(newValues[i]);
        const float previousValue = previousValues_[i];

        // Choose smoothing factor based on value direction
        const float smoothing = (newValue > previousValue) ? upwardSmoothing_ : downwardSmoothing_;

        // Apply exponential smoothing
        smoothedValues_[i] = previousValue * (1.0f - smoothing) + newValue * smoothing;
        previousValues_[i] = smoothedValues_[i];
    }
}

void ValueSmoother::update(const std::vector<uint8_t>& newValues) {
    update(newValues.data(), newValues.size());
}

uint8_t ValueSmoother::getSmoothedValue(size_t index) const {
    assert(index < size_ && "Index out of bounds");
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
    std::fill(previousValues_.begin(), previousValues_.end(), 0.0f);
}