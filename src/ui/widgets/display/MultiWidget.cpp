#include "MultiWidget.h"

MultiWidget::MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs) {}

void MultiWidget::onDrawStatic() {
    getLcd()->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                        kBorderColor);
}

void MultiWidget::onDraw(bool forceRedraw) {
    clearDirty();
}
