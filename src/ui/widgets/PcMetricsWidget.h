#pragma once

#include <string>

#include "services/PcMetrics.h"
#include "ui/widgets/Widget.h"

class PcMetricsWidget : public Widget {
   public:
    PcMetricsWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                    PcMetrics& pcMetrics);

    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

   private:
    PcMetrics& pcMetrics_;
    uint8_t lastCpuLoad_ = 255;  // Invalid initial value to force first draw
    uint8_t lastGpuLoad_ = 255;  // Invalid initial value to force first draw
};