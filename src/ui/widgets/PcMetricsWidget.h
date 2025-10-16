#pragma once

#include <string>

#include "SingleValueWidget.h"
#include "services/PcMetrics.h"
#include "ui/DisplayContext.h"
#include "ui/widgets/ThreadsWidget.h"
#include "ui/widgets/Widget.h"
#include <config/AppConfigInterface.h>

class PcMetricsWidget : public Widget {
   public:
    PcMetricsWidget(DisplayContext& context, const Dimensions& dims,
                    uint32_t updateIntervalMs, PcMetrics& pcMetrics, AppConfigInterface& config);

    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

   private:
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    AppConfigInterface& config_;

    unsigned long lastUpdateTimestamp_ = 0;

    std::unique_ptr<ThreadsWidget> threadsWidget_;
    std::unique_ptr<SingleValueWidget> cpuLoadWidget_;
    std::unique_ptr<SingleValueWidget> gpu3dWidget_;
    std::unique_ptr<SingleValueWidget> gpuComputeWidget_;
};