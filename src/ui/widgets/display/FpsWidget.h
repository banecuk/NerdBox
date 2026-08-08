#pragma once

#include <atomic>
#include <functional>

#include "core/events/EventTypes.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Displays the GPU fullscreen FPS in a compact square in the bottom-right area
// of the main screen, just above the ClockWidget.
// The widget is hidden whenever:
//   - data is not yet available (pcMetrics_.freshness.available() == false), or
//   - the reported FPS value is -1 (no fullscreen app running).
//
// Optionally tappable: pass an action + callback (mirrors ButtonWidget) to
// have a tap publish an EventType, e.g. to navigate to the game screen. The
// tile stays tappable even while showing the "---" placeholder.
class FpsWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    FpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
              uint32_t updateIntervalMs, PcMetrics& pcMetrics, EventType action = EventType::NONE,
              ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    PcMetrics& pcMetrics_;
    EventType action_;
    ActionCallback callback_;
    DataFreshnessGuard freshnessGuard_;

    int16_t lastDrawnFps_ = -2;  // sentinel so the first draw always renders
    bool lastVisible_ = false;

    void renderFps(int16_t fps);
    void renderPlaceholder();
    void clearValueArea();
    void clearArea();
};
