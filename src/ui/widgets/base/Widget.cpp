#include "Widget.h"

Widget::Widget(const Dimensions& dims, uint32_t updateIntervalMs)
    : dimensions_(dims), updateIntervalMs_(updateIntervalMs) {
    if (!dimensions_.isValid()) {
        state_ = WidgetInterface::State::ERROR;
    }
}

Widget::~Widget() {
    cleanUp();
}

void Widget::initialize(DisplayContext& context) {
    if (state_ != WidgetInterface::State::UNINITIALIZED) {
        return;  // Already initialized
    }

    try {
        context_ = &context;
        lcd_ = &context.getDisplay();
        logger_ = &context.getLogger();
        lastUpdateTimeMs_ = millis();

        // Call derived class initialization
        onInitialize();

        isInitialized_ = true;
        state_ = isVisible_ ? WidgetInterface::State::READY : WidgetInterface::State::HIDDEN;
        drawStatic();

    } catch (const std::exception& e) {
        state_ = WidgetInterface::State::ERROR;
        if (logger_) {
            logger_->errorf("Widget init failed: %s", e.what());
        }
        throw;
    }
}

void Widget::drawStatic() {
    if (state_ == WidgetInterface::State::ERROR ||
        state_ == WidgetInterface::State::UNINITIALIZED || !isVisible_) {
        return;
    }

    if (!lcd_) {
        state_ = WidgetInterface::State::ERROR;
        return;
    }

    try {
        onDrawStatic();
        isStaticDrawn_ = true;
        isDirty_ = false;
    } catch (const std::exception& e) {
        state_ = WidgetInterface::State::ERROR;
        if (logger_) {
            logger_->errorf("Widget drawStatic failed: %s", e.what());
        }
    }
}

void Widget::draw(bool forceRedraw) {
    if (!canDraw()) {
        return;
    }

    try {
        bool shouldDraw = forceRedraw || isDirty_ || needsUpdate();

        if (shouldDraw) {
            if (forceRedraw) {
                drawStatic();  // Redraw static if forced
            }

            onDraw(forceRedraw || isDirty_);
            lastUpdateTimeMs_ = millis();
            isDirty_ = false;
        }
    } catch (const std::exception& e) {
        state_ = WidgetInterface::State::ERROR;
        if (logger_) {
            logger_->errorf("Widget draw failed: %s", e.what());
        }
    }
}

void Widget::cleanUp() {
    if (state_ == WidgetInterface::State::UNINITIALIZED) {
        return;
    }

    if (logger_) {
        logger_->debugf("Cleaning up widget at (%d, %d)", dimensions_.x, dimensions_.y);
    }

    try {
        onCleanUp();
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->errorf("Widget cleanup failed: %s", e.what());
        }
    }

    state_ = WidgetInterface::State::UNINITIALIZED;
    isInitialized_ = false;
    isStaticDrawn_ = false;
    isDirty_ = false;
    isStale_ = false;

    lcd_ = nullptr;
    logger_ = nullptr;
    context_ = nullptr;
}

bool Widget::setVisible(bool visible) {
    if (state_ == WidgetInterface::State::UNINITIALIZED ||
        state_ == WidgetInterface::State::ERROR) {
        return false;
    }

    if (isVisible_ == visible) {
        return false;
    }

    isVisible_ = visible;

    if (visible) {
        state_ = WidgetInterface::State::READY;
        drawStatic();
    } else {
        state_ = WidgetInterface::State::HIDDEN;
        // Clear widget area
        if (lcd_) {
            lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                           TFT_BLACK);
        }
    }

    return true;
}

bool Widget::needsUpdate() const {
    if (updateIntervalMs_ == 0 || !isInitialized_ || !isVisible_) {
        return false;
    }
    return (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

bool Widget::isInitialized() const {
    return isInitialized_ && state_ != WidgetInterface::State::ERROR &&
           state_ != WidgetInterface::State::UNINITIALIZED;
}

bool Widget::isValid() const {
    return state_ != WidgetInterface::State::ERROR &&
           state_ != WidgetInterface::State::UNINITIALIZED && dimensions_.isValid();
}

bool Widget::canDraw() const {
    return isInitialized_ && isVisible_ && lcd_ != nullptr;
}