#include "PcDataCompositeWidget.h"

PcDataCompositeWidget::PcDataCompositeWidget(const WidgetInterface::Dimensions& dims,
                                             uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics), freshnessGuard_(pcMetrics.freshness) {}

void PcDataCompositeWidget::initAndDrawWidget(MetricWidget& widget) {
    widget.initialize(getContext());
    widget.drawStatic();
    widget.forceRefreshAndDraw();
}

void PcDataCompositeWidget::onDrawStatic() {
    if (hasFreshData()) {
        drawFreshStatic();
    } else {
        clearChildren();
        isStaticDrawn_ = false;
        lastUpdateTimestamp_ = 0;
        drawNoDataMessage();
    }
}

void PcDataCompositeWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool currentlyHasFreshData = hasFreshData();
    const bool stateChanged = (wasFreshData_ != currentlyHasFreshData);

    if (stateChanged) {
        clearChildren();
        isStaticDrawn_ = false;
        lastUpdateTimestamp_ = 0;
        if (currentlyHasFreshData) {
            drawStatic();  // -> onDrawStatic() -> drawFreshStatic()
        } else {
            drawNoDataMessage();
        }
        clearDirty();
        wasFreshData_ = currentlyHasFreshData;
    }

    if (currentlyHasFreshData && pcMetrics_.freshness.lastUpdateMs() != lastUpdateTimestamp_) {
        ensureChildWidgetsCreated();
    }

    const bool needsRedraw = forceRedraw || isDirty() || needsUpdate();
    if (currentlyHasFreshData && needsRedraw) {
        drawDynamicData();
        clearDirty();
        lastUpdateTimestamp_ = pcMetrics_.freshness.lastUpdateMs();
    }

}

bool PcDataCompositeWidget::needsUpdate() const {
    if (!isInitialized())
        return false;
    if (hasFreshData() != wasFreshData_)
        return true;
    // updateIntervalMs_ only bounds the *maximum* rate (Widget::needsUpdate()'s
    // contract); the timestamp comparison is what actually decides whether
    // there's new data to draw. A time-only OR here forced a full repaint
    // every tick regardless of whether anything changed.
    // `!=`, not `>`: after the 49.7-day millis() wrap new timestamps are
    // smaller than the cached one, and `>` froze the tiles (B53).
    return pcMetrics_.freshness.lastUpdateMs() != lastUpdateTimestamp_;
}
