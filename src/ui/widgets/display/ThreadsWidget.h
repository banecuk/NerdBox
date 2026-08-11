#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "config/AppSettings.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/ApplicationMetrics.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/ValueSmoother.h"

class ThreadsWidget : public Widget {
 public:
    ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                  uint32_t updateIntervalMs, PcMetrics& pcMetrics, const AppSettings& config,
                  ApplicationMetrics& systemMetrics);

    void initialize(DisplayContext& context) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    const AppSettings& config_;
    ApplicationMetrics& systemMetrics_;
    DataFreshnessGuard freshnessGuard_;

    uint16_t barWidth_;
    std::vector<uint16_t> previousBarHeights_;
    std::vector<uint16_t>
        previousColors_;  // tracks last drawn color per bar for threshold change detection

    std::unique_ptr<ValueSmoother> valueSmoother_;
    std::vector<uint8_t> smoothedThreadLoads_;

    // Tracks the freshness state as of the last draw, so needsUpdate() can
    // (a) force one redraw on a fresh<->stale transition and (b) otherwise
    // stop ticking every kThreadsRefreshMs while stale — there's nothing new
    // to animate, and a "No Data" message is already shown in place of bars.
    bool wasFresh_ = true;

    void drawBars();
    void drawNoDataMessage();
    void updateSmoothedValues();
};