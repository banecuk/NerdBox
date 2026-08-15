#include "MetricColorPolicy.h"

namespace MetricColorPolicy {

uint8_t normalizedPercent(int value, float lowerThreshold, float upperThreshold,
                          bool reverseThresholds) {
    float normalized;

    if (reverseThresholds) {
        // REVERSE LOGIC: warning color for LOW values
        if (value >= upperThreshold) {
            normalized = 0.0f;  // Good (green) when value is HIGH
        } else if (value <= lowerThreshold) {
            normalized = 100.0f;  // Bad (red) when value is LOW
        } else {
            const float range = upperThreshold - lowerThreshold;
            normalized = range <= 0.0f ? 0.0f : 100.0f * (upperThreshold - value) / range;
        }
    } else {
        // NORMAL LOGIC: warning color for HIGH values
        if (value <= lowerThreshold) {
            normalized = 0.0f;
        } else if (value >= upperThreshold) {
            normalized = 100.0f;
        } else {
            const float range = upperThreshold - lowerThreshold;
            normalized = range <= 0.0f ? 0.0f : 100.0f * (value - lowerThreshold) / range;
        }
    }

    return static_cast<uint8_t>(normalized);
}

}  // namespace MetricColorPolicy
