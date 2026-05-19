#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class ValueSmoother {
 public:
    ValueSmoother(size_t size, float upwardSmoothing, float downwardSmoothing);

    void update(const uint8_t* newValues, size_t count);
    void update(const std::vector<uint8_t>& newValues);

    uint8_t getSmoothedValue(size_t index) const;
    void getSmoothedValues(uint8_t* output, size_t count) const;
    void getSmoothedValues(std::vector<uint8_t>& output) const;

    void reset();
    size_t size() const { return size_; }
    bool hasPrevious() const { return hasPrevious_; }

 private:
    size_t size_;
    float upwardSmoothing_;
    float downwardSmoothing_;
    std::vector<float> smoothedValues_;
    std::vector<float> previousValues_;
    bool hasPrevious_ = false;

    static constexpr float ROUNDING_OFFSET = 0.5f;
};