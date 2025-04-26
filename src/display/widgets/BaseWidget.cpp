#include "BaseWidget.h"

BaseWidget::BaseWidget(const Dimensions& dims, uint32_t updateIntervalMs)
    : dimensions_(dims),
      updateIntervalMs_(updateIntervalMs),
      lastUpdateTimeMs_(0)  // Ensure last update time starts at 0
{
    // Logger and LCD are set during initialize
}

void BaseWidget::initialize(LGFX* lcd, ILogger& logger) {
    if (!lcd) {
        // Optionally, log an error if logger is available, or handle differently
        // For now, just prevent initialization with null LCD
        return;
    }
    lcd_ = lcd;
    logger_ = &logger;             // Store the logger reference
    lastUpdateTimeMs_ = millis();  // Set initial time on initialize
    initialized_ = true;
    // Derived classes can override this to add specific initialization
    // but should call BaseWidget::initialize() or replicate its logic.
    // logger_->debug("BaseWidget::initialize for widget at (%d, %d)", dimensions_.x,
    // dimensions_.y);
}

// Default cleanup is empty, derived classes can override if needed
void BaseWidget::cleanUp() {
    if (logger_) {
        // logger_->debug("BaseWidget::cleanUp for widget at (%d, %d)", dimensions_.x,
        // dimensions_.y);
    }
    initialized_ = false;
    // Reset pointers, though lcd_ is typically managed by ScreenManager
    lcd_ = nullptr;
    logger_ = nullptr;
}

void BaseWidget::setUpdateInterval(uint32_t intervalMs) {
    updateIntervalMs_ = intervalMs;
}

bool BaseWidget::needsUpdate() const {
    // If interval is 0, it never needs time-based updates (only forced redraw)
    if (updateIntervalMs_ == 0) {
        return false;
    }
    return (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

IWidget::Dimensions BaseWidget::getDimensions() const { return dimensions_; }

// Note: draw() and handleTouch() are not implemented here as they
// depend on the specific widget's appearance and behavior.
// Derived classes MUST provide their own implementations.