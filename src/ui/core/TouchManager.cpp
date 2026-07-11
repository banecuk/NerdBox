#include "TouchManager.h"

TouchManager::TouchManager(LGFX& display, LoggerInterface& logger, const AppSettings& config)
    : display_(display),
      logger_(logger),
      config_(config),
      lastTouchTime_(0),
      suppressUntilTime_(0),
      debounceIntervalMs_(config.uiTouchDebounceIntervalMs) {}

TouchManager::TouchPoint TouchManager::readTouch() {
    // Read touch coordinates first — debounce/suppression only matters (and
    // should only be logged) when a finger is actually on the glass. Checking
    // it before reading the touch meant this logged on every poll while idle.
    int32_t x = 0, y = 0;
    if (!display_.getTouch(&x, &y)) {
        return TouchPoint();  // No touch detected
    }

    if (shouldDebounce()) {
        logger_.debug("[TouchManager] Touch ignored due to debounce");
        return TouchPoint();  // Invalid touch
    }

    // Validate coordinates
    if (!isValidCoordinate(x, y)) {
        logger_.warningf("[TouchManager] Invalid coordinates: (%d, %d)", x, y);
        return TouchPoint();  // Invalid coordinates
    }

    // Valid touch detected - update timestamp and return
    lastTouchTime_ = millis();
    logger_.debugf("[TouchManager] Touch detected at (%d, %d)", x, y);

    return TouchPoint(x, y);
}

bool TouchManager::isValidCoordinate(int32_t x, int32_t y) const {
    return (x >= 0 && x < display_.width() && y >= 0 && y < display_.height());
}

bool TouchManager::shouldDebounce() const {
    unsigned long currentTime = millis();
    if (static_cast<long>(currentTime - suppressUntilTime_) < 0) {
        return true;  // still within an explicit suppression window
    }
    return (currentTime - lastTouchTime_) < debounceIntervalMs_;
}

void TouchManager::suppressFor(uint32_t durationMs) {
    suppressUntilTime_ = millis() + durationMs;
    logger_.debug("[TouchManager] Touch suppressed");
}