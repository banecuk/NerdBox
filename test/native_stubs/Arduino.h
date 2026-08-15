#pragma once

// Minimal stand-in for the Arduino core header, present only on PlatformIO's
// `native` platform build path (`[env:native]`), which has no Arduino/ESP-IDF
// SDK to link against. Provides just enough surface for host-testable
// classes that transitively include <Arduino.h> (PcMetrics.h, PublishedFlag.h,
// LoggerInterface.h, ...) to compile: String, millis(), and the min/max
// template helpers Arduino normally injects into the global namespace.
//
// Never included on the real firmware build — WT32-SC01-PLUS-debug/-release
// use the real framework-arduinoespressif32 Arduino.h via PlatformIO's own
// include paths, which take priority there.

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using String = std::string;

inline unsigned long millis() {
    return 0;
}

template <typename T>
T min(T a, T b) {
    return a < b ? a : b;
}

template <typename T>
T max(T a, T b) {
    return a > b ? a : b;
}
