#pragma once

#include <cstdint>

#include "config/AppConfigInterface.h"
#include "config/LgfxConfig.h"
#include "utils/LoggerInterface.h"

class TouchManager {
 public:
    struct TouchPoint {
        int32_t x;
        int32_t y;
        bool valid;

        TouchPoint() : x(0), y(0), valid(false) {}
        TouchPoint(int32_t x_, int32_t y_) : x(x_), y(y_), valid(true) {}
    };

    explicit TouchManager(LGFX& display, LoggerInterface& logger, AppConfigInterface& config);
    ~TouchManager() = default;

    // Delete copy/move operations
    TouchManager(const TouchManager&) = delete;
    TouchManager& operator=(const TouchManager&) = delete;
    TouchManager(TouchManager&&) = delete;
    TouchManager& operator=(TouchManager&&) = delete;

    TouchPoint readTouch();

    bool isValidCoordinate(int32_t x, int32_t y) const;

    void resetDebounce();

    unsigned long getTimeSinceLastTouch() const;

 private:
    LGFX& display_;
    LoggerInterface& logger_;
    AppConfigInterface& config_;

    unsigned long lastTouchTime_;
    uint32_t debounceIntervalMs_;

    bool shouldDebounce() const;
};