#pragma once

#include <vector>

#include "services/cpuClock/CpuClockData.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Content widget for CpuClockScreen: a grid of per-core clock speeds (MHz)
// plus a bus-speed line below it. Reads a const CpuClockData&, gated on
// freshness.available() the same way every other widget checks
// DataFreshnessGuard — this data only arrives while the screen itself is
// active (see CpuClockStreamJob's screen gate), so "stale" here just means
// "just opened the screen, first event hasn't landed yet."
class CpuClockWidget : public Widget {
 public:
    CpuClockWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                   uint32_t updateIntervalMs, const CpuClockData& data);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint16_t kBusLineHeight = 32;

    // Fixed at 4 regardless of coreCount_: cell width (dimensions_.width/4)
    // stays generous enough for the larger metric font at any core count —
    // only cellHeight_ shrinks as rows grow for higher core counts (screens
    // must support at least 20 cores, up to AppConfig::Limits::kMaxThreads).
    static constexpr uint8_t kColumns = 4;

    const CpuClockData& data_;
    DataFreshnessGuard freshnessGuard_;

    // 0 until the first CoreClocksMHz payload arrives; then latched to the
    // reported core count for the widget's lifetime, mirroring
    // ThreadsWidget::ensureLayoutInitialized().
    uint8_t coreCount_ = 0;
    uint16_t cellWidth_ = 0;
    uint16_t cellHeight_ = 0;
    uint16_t gridRows_ = 0;

    std::vector<float> previousClockMHz_;
    float previousBusSpeedMHz_ = -1.0f;
    bool wasFresh_ = false;

    bool ensureLayoutInitialized();
    void drawGrid(bool forceRedraw);
    void drawBusSpeed(bool forceRedraw);
    void drawNoDataMessage();
    void drawCell(uint8_t index, float mhz);
};
