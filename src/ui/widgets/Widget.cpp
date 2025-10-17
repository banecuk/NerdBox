#include "Widget.h"

Widget::Widget(const Dimensions& dims, uint32_t updateIntervalMs)
    : dimensions_(dims), updateIntervalMs_(updateIntervalMs), lastUpdateTimeMs_(0) {}

void Widget::initialize(DisplayContext& context) {
    // if (!context.getDisplay()) {
    //     Serial.println("Widget initialization failed - null display");
    //     return;
    // }
    // if (!context.getLogger()) {
    //     Serial.println("Widget initialization failed - null logger");
    //     return;
    // }
    lcd_ = &context.getDisplay();
    logger_ = &context.getLogger();
    lastUpdateTimeMs_ = millis();
    initialized_ = true;
    drawStatic();
    isStaticDrawn_ = true;
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
    isStaticDrawn_ = false;
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

WidgetInterface::Dimensions Widget::getDimensions() const { return dimensions_; }
