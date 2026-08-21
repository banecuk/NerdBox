#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "config/AppSettings.h"
#include "core/events/EventTypes.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/ThreadStaggerScheduler.h"
#include "ui/widgets/base/Widget.h"
#include "utils/ApplicationMetrics.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/ValueSmoother.h"

class ThreadsWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                  uint32_t updateIntervalMs, PcMetrics& pcMetrics, const AppSettings& config,
                  ApplicationMetrics& systemMetrics,
                  EventType action = EventType::SHOW_CPU_CLOCK, ActionCallback callback = nullptr);

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
    EventType action_;
    ActionCallback callback_;
    DataFreshnessGuard freshnessGuard_;

    uint16_t barWidth_ = 0;
    std::vector<uint16_t> previousBarHeights_;
    std::vector<uint16_t>
        previousColors_;  // tracks last drawn color per bar for threshold change detection

    std::unique_ptr<ValueSmoother> valueSmoother_;
    std::vector<uint8_t> smoothedThreadLoads_;

    // Staggers each bar's release onto the newly-arrived raw values (see
    // ThreadStaggerScheduler); stagedTargets_ is what actually feeds
    // valueSmoother_ — a bar not yet released keeps chasing its previous
    // target instead of jumping to the new one.
    ThreadStaggerScheduler stagger_;
    std::vector<uint8_t> stagedTargets_;

    // 0 until the first CoreLoads payload arrives; then latched to the
    // reported thread count for the widget's lifetime — see
    // ensureLayoutInitialized().
    uint8_t coreCount_ = 0;

    // Tracks the freshness state as of the last draw, so needsUpdate() can
    // (a) force one redraw on a fresh<->stale transition and (b) otherwise
    // stop ticking every kThreadsRefreshMs while stale — there's nothing new
    // to animate, and a "No Data" message is already shown in place of bars.
    bool wasFresh_ = true;

    // Sizes barWidth_/the per-bar vectors/the smoother from
    // pcMetrics_.cpu_core_count the first time it's non-zero, since the
    // widget is constructed at boot, before any PC-metrics data has arrived.
    // Returns true once layout is known (whether just-initialized or already
    // latched from an earlier call).
    bool ensureLayoutInitialized();
    void drawBars();
    void drawNoDataMessage();
    void updateSmoothedValues();
};