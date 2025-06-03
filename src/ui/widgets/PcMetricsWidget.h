#pragma once

#include <string>

#include "services/PcMetrics.h"
#include "ui/widgets/ThreadsWidget.h"
#include "ui/widgets/Widget.h"
#include "SingleValueWidget.h"

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

    std::unique_ptr<ThreadsWidget> threadsWidget_;
    std::unique_ptr<SingleValueWidget> cpuLoadWidget_;
    std::unique_ptr<SingleValueWidget> gpu3dWidget_;
    std::unique_ptr<SingleValueWidget> gpuComputeWidget_;
};