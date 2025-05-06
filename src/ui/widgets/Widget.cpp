#include "Widget.h"

Widget::Widget(const Dimensions& dims, uint32_t updateIntervalMs)
    : dimensions_(dims),
      updateIntervalMs_(updateIntervalMs),
      lastUpdateTimeMs_(0)
{

}

void Widget::initialize(LGFX* lcd, ILogger& logger) {
    if (!lcd) {
        logger.error("Widget initialization failed - null LCD");
        return;
    }
    lcd_ = lcd;
    logger_ = &logger;
    lastUpdateTimeMs_ = millis();
    initialized_ = true;
}

void Widget::cleanUp() {
    if (logger_) {
        logger_->debugf("Widget::cleanUp for widget at (%d, %d)", dimensions_.x,
                        dimensions_.y);
    }
    initialized_ = false;
    lcd_ = nullptr;
    logger_ = nullptr;
}

void Widget::setUpdateInterval(uint32_t intervalMs) { updateIntervalMs_ = intervalMs; }

bool Widget::needsUpdate() const {
    // If interval is 0, it never needs time-based updates (only forced redraw)
    if (updateIntervalMs_ == 0) {
        return false;
    }
    return (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

IWidget::Dimensions Widget::getDimensions() const { return dimensions_; }

// Note: draw() and handleTouch() are not implemented here as they
// depend on the specific widget's appearance and behavior.
// Derived classes MUST provide their own implementations.