#pragma once

#include "services/PcMetrics.h"
#include "ui/DisplayContext.h"
#include "ui/widgets/Widget.h"

class ThreadsWidget : public Widget {
   public:
    ThreadsWidget(DisplayContext& context, const Dimensions& dims,
                  uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

   private:
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    unsigned long lastUpdateTimestamp_ = 0;
    void drawBars(bool forceRedraw);
    uint16_t previousBarHeights_[20] = {0};
};