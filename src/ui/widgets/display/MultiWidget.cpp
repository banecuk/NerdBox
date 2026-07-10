#include "MultiWidget.h"

MultiWidget::MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs) {}

void MultiWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    getLcd()->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                        kBorderColor);

    isStaticDrawn_ = true;
    clearDirty();
}

void MultiWidget::onDraw(bool forceRedraw) {
    clearDirty();
}
