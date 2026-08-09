#pragma once

#include <memory>
#include <vector>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"

// Multifunctional content area on MainScreen — sits below the AirQualityWidget,
// left of the FpsWidget, and above the ClockWidget / NetworkWidget row.
// Container: owns one or more candidate sub-widgets and shows whichever one
// is active, so it can later switch content based on which data is most
// relevant. For now there is a single candidate (HistorySparklineWidget) and
// no switching rules — those land once more candidates exist.
class MultiWidget : public Widget {
 public:
    MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
               PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onInitialize() override;
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;
    void onCleanUp() override;

 private:
    PcMetrics& pcMetrics_;
    std::vector<std::unique_ptr<WidgetInterface>> candidates_;
    size_t activeIndex_ = 0;
};
