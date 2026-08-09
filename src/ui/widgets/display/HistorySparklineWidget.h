#pragma once

#include <cstdint>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/PsramRingHistory.h"

// CPU and GPU load over the last ~5 minutes, rendered as two stacked mini
// sparklines (CPU, GPU), borderless. First (and currently only) candidate
// hosted by MultiWidget; see MultiWidget.h for the container.
class HistorySparklineWidget : public Widget {
 public:
    HistorySparklineWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                           PcMetrics& pcMetrics);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // 5 minutes of history at one sample/second.
    static constexpr size_t kHistorySize = 300;
    static constexpr uint32_t kSampleIntervalMs = 1000;
    static constexpr uint16_t kColWidth = 1;
    static constexpr uint16_t kCaptionWidth = 40;
    static constexpr uint16_t kRowMargin = 2;
    static constexpr uint16_t kRowGap = 2;
    static constexpr uint8_t kScaleMax = 100;

    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;

    PsramRingHistory<uint8_t> cpuLoadHistory_;
    PsramRingHistory<uint8_t> gpuLoadHistory_;
    uint32_t lastSampleMs_ = 0;

    uint16_t captionX_ = 0;
    uint16_t plotX_ = 0;
    uint16_t plotWidth_ = 0;
    uint16_t rowHeight_ = 0;
    uint16_t cpuPlotY_ = 0;
    uint16_t gpuPlotY_ = 0;

    uint8_t lastCpuCol_[kHistorySize] = {0};
    uint8_t lastGpuCol_[kHistorySize] = {0};

    void computeLayout();
    void sampleIfNeeded();
    void drawRow(uint16_t plotY, const PsramRingHistory<uint8_t>& history, uint8_t* lastCol,
                uint16_t color, uint16_t lowColor, bool forceFullRepaint);
};
