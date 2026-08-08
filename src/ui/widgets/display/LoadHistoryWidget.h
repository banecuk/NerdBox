#pragma once

#include <atomic>
#include <cstdint>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/RingHistory.h"

// CPU + GPU load history strip for the game screen — fixed 0-100% scale dual
// sparkline. CPU load (0xC618, near-white) is drawn as a filled bar; GPU load
// (0xB471, muted desaturated red — same GPU accent color used elsewhere in the app) as
// a bright marker line on top of it. One sample recorded per PcMetrics fetch,
// same timestamp-gated sampling as GameFpsWidget.
//
// kHistorySize is sized so kCaptionWidth + kHistorySize*kColWidth + margin
// exactly fills the widget's designed 480px width (see computePlotLayout) —
// the plot must reach the right screen edge, not stop short of it.
class LoadHistoryWidget : public Widget {
 public:
    LoadHistoryWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                      uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr size_t kHistorySize = 219;
    static constexpr uint16_t kColWidth = 2;
    static constexpr uint16_t kCaptionWidth = 40;
    static constexpr uint16_t kRightMargin = 2;

    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;

    RingHistory<uint8_t, kHistorySize> cpuHistory_;
    RingHistory<uint8_t, kHistorySize> gpuHistory_;
    unsigned long lastSampledTimestamp_ = 0;

    uint8_t lastCpuHeight_[kHistorySize] = {0};
    uint8_t lastGpuHeight_[kHistorySize] = {0};

    uint16_t plotX_ = 0;
    uint16_t plotY_ = 0;
    uint16_t plotWidth_ = 0;
    uint16_t plotHeight_ = 0;

    void computePlotLayout();
    void sampleIfNeeded();
    void drawPlot(bool forceFullRepaint);
};
