#pragma once

#include <string>

#include "services/PcMetrics.h"
#include "ui/widgets/Widget.h"

class PcMetricsWidget : public Widget {
   public:
    PcMetricsWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                    PcMetrics& pcMetrics);

    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

   private:
    PcMetrics& pcMetrics_;
    unsigned long lastUpdateTimestamp_ = 0; 
};