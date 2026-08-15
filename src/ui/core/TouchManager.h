#pragma once

#include <cstdint>

#include "config/AppSettings.h"
#include "config/LgfxConfig.h"
#include "utils/logging/LoggerInterface.h"

class TouchManager {
 public:
    struct TouchPoint {
        int32_t x;
        int32_t y;
        bool valid;

        TouchPoint() : x(0), y(0), valid(false) {}
        TouchPoint(int32_t x_, int32_t y_) : x(x_), y(y_), valid(true) {}
    };

    explicit TouchManager(LGFX& display, LoggerInterface& logger, const AppSettings& config);
    ~TouchManager() = default;

    // Delete copy/move operations
    TouchManager(const TouchManager&) = delete;
    TouchManager& operator=(const TouchManager&) = delete;
    TouchManager(TouchManager&&) = delete;
    TouchManager& operator=(TouchManager&&) = delete;

    TouchPoint readTouch();

    bool isValidCoordinate(int32_t x, int32_t y) const;

    // Suppresses all touches for durationMs from now. Single chokepoint for
    // anything that needs to blackout touch input (e.g. post-transition
    // cooldown) — replaces having callers keep their own cooldown timers.
    void suppressFor(uint32_t durationMs);

 private:
    LGFX& display_;
    LoggerInterface& logger_;
    const AppSettings& config_;

    unsigned long lastTouchTime_;
    unsigned long suppressUntilTime_;
    uint32_t debounceIntervalMs_;

    bool shouldDebounce() const;
};