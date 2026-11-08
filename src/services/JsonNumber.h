#pragma once

#include <ArduinoJson.h>

#include <cmath>
#include <limits>

// Reads a JSON number node as an integer type, safely for both integer and
// float nodes (B5).
//
// ArduinoJson's `node | 0` operator only returns the node's value when it
// is<int>() — a float node such as 61.0 or 42.5 silently yields the fallback
// instead. This is the same trap CLAUDE.md documents for FullscreenFps. Going
// through as<float>() reads either kind correctly.
//
// The value is multiplied by `scale` (e.g. 10 for the x10 fixed-point fields),
// rounded half away from zero so negative values (below-freezing temperatures)
// round symmetrically, then clamped to T's range so an out-of-range or negative
// value can't wrap (casting a negative float to an unsigned type is UB).
// Null, non-numeric and non-finite nodes read as 0.
namespace JsonNumber {

template <typename T>
T roundAs(JsonVariantConst node, float scale = 1.0f) {
    static_assert(std::numeric_limits<T>::is_integer, "roundAs<T> targets integer types");
    if (node.isNull() || !node.is<float>()) {
        return T{0};
    }
    const float value = node.as<float>() * scale;
    if (!std::isfinite(value)) {
        return T{0};
    }
    const float rounded = std::round(value);
    constexpr float kMin = static_cast<float>(std::numeric_limits<T>::min());
    constexpr float kMax = static_cast<float>(std::numeric_limits<T>::max());
    if (rounded <= kMin) return std::numeric_limits<T>::min();
    if (rounded >= kMax) return std::numeric_limits<T>::max();
    return static_cast<T>(rounded);
}

}  // namespace JsonNumber
