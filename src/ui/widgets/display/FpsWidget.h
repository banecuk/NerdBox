#pragma once

#include "config/AppConfigInterface.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"

// Displays the GPU fullscreen FPS in a compact square in the bottom-right area
// of the main screen, just above the ClockWidget.
// The widget is hidden whenever:
//   - data is not yet available (pcMetrics_.is_available == false), or
//   - the reported FPS value is -1 (no fullscreen app running).
class FpsWidget : public Widget {
 public:
    FpsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
              uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    PcMetrics& pcMetrics_;

    int16_t lastDrawnFps_ = -2;  // sentinel so the first draw always renders
    bool lastVisible_ = false;

    void renderFps(int16_t fps);
    void renderPlaceholder();
    void clearValueArea();
    void clearArea();
};
