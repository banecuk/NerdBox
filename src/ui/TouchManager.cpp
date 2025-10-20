#include "TouchManager.h"

TouchManager::TouchManager(LGFX& display, LoggerInterface& logger, AppConfigInterface& config)
    : display_(display),
      logger_(logger),
      config_(config),
      lastTouchTime_(0),
      debounceIntervalMs_(config.getUiTouchDebounceIntervalMs()) {}

TouchManager::TouchPoint TouchManager::readTouch() {
    // Check if we should debounce
    if (shouldDebounce()) {
        logger_.debug("[TouchManager] Touch ignored due to debounce");
        return TouchPoint();  // Invalid touch
    }

    // Read touch coordinates from display
    int32_t x = 0, y = 0;
    if (!display_.getTouch(&x, &y)) {
        return TouchPoint();  // No touch detected
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
    return (currentTime - lastTouchTime_) < debounceIntervalMs_;
}

void TouchManager::resetDebounce() {
    lastTouchTime_ = 0;
    logger_.debug("[TouchManager] Debounce timer reset");
}

unsigned long TouchManager::getTimeSinceLastTouch() const {
    return millis() - lastTouchTime_;
}