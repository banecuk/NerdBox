#pragma once

#include <atomic>
#include <cstdint>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/RingHistory.h"

// Large GPU fullscreen FPS readout with a scrolling history sparkline, shown
// on the game screen. One sample is recorded per PcMetrics fetch (gated on
// freshness.lastUpdateMs(), not a fixed timer) so the plot reflects real data
// cadence instead of duplicating/dropping samples.
//
// gpu_fps == -1 ("no fullscreen app running") is stored as-is and rendered as
// a gap in the sparkline rather than a zero dip; the big number shows "---".
class GameFpsWidget : public Widget {
 public:
    GameFpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                  uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr size_t kHistorySize = 160;
    static constexpr uint16_t kColWidth = 2;
    static constexpr int16_t kNoSample = -1;  // matches PcMetrics::gpu_fps "no app" sentinel
    static constexpr int16_t kScaleStep = 30;
    static constexpr int16_t kMinScale = 60;

    // Number block (left) layout
    static constexpr uint16_t kNumberBlockWidth = 140;

    // Sparkline plot layout — inset from the widget's right edge, leaving a
    // small margin and a separator gap after the number block.
    static constexpr uint16_t kPlotMarginTop = 16;
    static constexpr uint16_t kPlotMarginBottom = 14;
    static constexpr uint16_t kPlotLeftGap = 10;  // gap after number block
    static constexpr uint16_t kPlotRightMargin = 10;

    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;

    RingHistory<int16_t, kHistorySize> history_;
    unsigned long lastSampledTimestamp_ = 0;
    int16_t yMax_ = kMinScale;
    bool scaleChanged_ = true;  // force initial scale-label draw

    // Per-column cached bar height (px), so redraws only touch changed columns.
    uint8_t lastColHeight_[kHistorySize] = {0};

    int16_t lastDrawnFps_ = -2;  // sentinel so the first draw always renders
    bool lastVisible_ = false;

    uint16_t plotX_ = 0;
    uint16_t plotY_ = 0;
    uint16_t plotWidth_ = 0;
    uint16_t plotHeight_ = 0;

    void computePlotLayout();
    void sampleIfNeeded();
    void updateScale(int16_t sampleValue);
    void renderFpsNumber(int16_t fps);
    void renderPlaceholder();
    void clearNumberValueArea();
    void drawScaleLabels();
    void drawSparkline(bool forceFullRepaint);
};
