#include "MultiWidget.h"

#include "ui/widgets/display/HistorySparklineWidget.h"

MultiWidget::MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics) {}

void MultiWidget::onInitialize() {
    candidates_.push_back(
        std::make_unique<HistorySparklineWidget>(dimensions_, updateIntervalMs_, pcMetrics_));
    for (auto& candidate : candidates_)
        candidate->initialize(getContext());
    activeIndex_ = 0;
}

void MultiWidget::onDrawStatic() {
    if (activeIndex_ < candidates_.size())
        candidates_[activeIndex_]->drawStatic();
}

void MultiWidget::onDraw(bool forceRedraw) {
    if (activeIndex_ < candidates_.size())
        candidates_[activeIndex_]->draw(forceRedraw);
    clearDirty();
}

void MultiWidget::onCleanUp() {
    for (auto& candidate : candidates_)
        candidate->cleanUp();
    candidates_.clear();
}

bool MultiWidget::handleTouch(uint16_t x, uint16_t y) {
    if (activeIndex_ < candidates_.size())
        return candidates_[activeIndex_]->handleTouch(x, y);
    return false;
}
