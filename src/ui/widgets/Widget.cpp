#include "Widget.h"

Widget::Widget(const Dimensions& dims, uint32_t updateIntervalMs)
    : dimensions_(dims), updateIntervalMs_(updateIntervalMs), lastUpdateTimeMs_(0) {}

void Widget::initialize(LGFX* lcd, ILogger& logger) {
    if (!lcd) {
        logger.error("Widget initialization failed - null LCD");
        return;
    }
    lcd_ = lcd;
    logger_ = &logger;
    lastUpdateTimeMs_ = millis();
    initialized_ = true;
    drawStatic();
    staticDrawn_ = true;
}

void Widget::drawStatic() {
    // if (logger_) {
    //     logger_->debug("Widget::drawStatic - Default implementation");
    // }
}

void Widget::cleanUp() {
    if (logger_) {
        logger_->debugf("Widget::cleanUp for widget at (%d, %d)", dimensions_.x,
                        dimensions_.y);
    }
    initialized_ = false;
    staticDrawn_ = false;
    lcd_ = nullptr;
    logger_ = nullptr;
}

void Widget::setUpdateInterval(uint32_t intervalMs) { updateIntervalMs_ = intervalMs; }

bool Widget::needsUpdate() const {
    if (updateIntervalMs_ == 0) {
        return false;
    }
    return (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

IWidget::Dimensions Widget::getDimensions() const { return dimensions_; }
