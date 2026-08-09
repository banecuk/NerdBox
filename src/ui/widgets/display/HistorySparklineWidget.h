#pragma once

#include <cstdint>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/PsramRingHistory.h"

// CPU/GPU/RAM/VRAM load over the last few minutes, rendered as four mini
// sparklines in a 2x2 grid — CPU/GPU on the left, RAM/VRM on the right.
// Borderless. First (and currently only) candidate hosted by MultiWidget;
// see MultiWidget.h for the container.
class HistorySparklineWidget : public Widget {
 public:
    HistorySparklineWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           PcMetrics& pcMetrics);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint32_t kSampleIntervalMs = 1000;
    static constexpr uint16_t kColWidth = 1;
    static constexpr uint16_t kCaptionWidth = 40;
    static constexpr uint16_t kRowMargin = 2;
    static constexpr uint16_t kRowGap = 2;
    static constexpr uint8_t kScaleMax = 100;
    static constexpr uint8_t kCpuGpuLowThreshold = 20;
    static constexpr uint8_t kRamVramLowThreshold = 60;
    // Any metric at or above this is flagged with the shared alert color,
    // regardless of which of the four sparklines it is.
    static constexpr uint8_t kHighUsageThreshold = 90;

    // Upper bound for the fixed-size redraw-cache arrays below. The actual
    // column count (and matching ring-buffer capacity) is derived from the
    // widget's real width in plotWidthForHalf()/computeLayout() — this is
    // just a ceiling comfortably above any width this widget is ever given.
    static constexpr size_t kMaxColumns = 256;

    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;

    PsramRingHistory<uint8_t> cpuLoadHistory_;
    PsramRingHistory<uint8_t> gpuLoadHistory_;
    PsramRingHistory<uint8_t> ramLoadHistory_;
    PsramRingHistory<uint8_t> vramLoadHistory_;
    uint32_t lastSampleMs_ = 0;

    uint16_t leftCaptionX_ = 0;
    uint16_t leftPlotX_ = 0;
    uint16_t leftPlotWidth_ = 0;
    uint16_t leftCols_ = 0;

    uint16_t rightCaptionX_ = 0;
    uint16_t rightPlotX_ = 0;
    uint16_t rightPlotWidth_ = 0;
    uint16_t rightCols_ = 0;

    uint16_t rowHeight_ = 0;
    uint16_t topPlotY_ = 0;
    uint16_t bottomPlotY_ = 0;

    // Whether the widget (captions + sparklines) was showing on the last
    // draw — lets onDraw detect the fresh/stale transition so it can blank
    // the area exactly once instead of leaving a frozen last-known plot on
    // screen while PcMetrics is stale, and redraw captions once data returns.
    bool lastHasData_ = true;

    uint8_t lastCpuCol_[kMaxColumns] = {0};
    uint8_t lastGpuCol_[kMaxColumns] = {0};
    uint8_t lastRamCol_[kMaxColumns] = {0};
    uint8_t lastVramCol_[kMaxColumns] = {0};

    // Plot width (in px) available to the left (CPU/GPU) or right (RAM/VRM)
    // half of the widget, after reserving kCaptionWidth for the label. Static
    // (and takes dims explicitly) so it can size the ring-buffer capacities
    // in the constructor's initializer list, before computeLayout() runs.
    static uint16_t plotWidthForHalf(const WidgetInterface::Dimensions& dims, bool leftHalf);
    void computeLayout();
    void sampleIfNeeded();
    void drawRow(uint16_t plotX, uint16_t plotY, uint16_t cols,
                const PsramRingHistory<uint8_t>& history, uint8_t* lastCol, uint16_t color,
                uint16_t lowColor, uint8_t lowThreshold, bool forceFullRepaint);
};
