#pragma once

#include <memory>
#include <vector>

#include "config/AppConfigInterface.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/ApplicationMetrics.h"
#include "utils/ValueSmoother.h"

class ThreadsWidget : public Widget {
 public:
    ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                  uint32_t updateIntervalMs, PcMetrics& pcMetrics, AppConfigInterface& config,
                  ApplicationMetrics& systemMetrics);

    void initialize(DisplayContext& context) override;
    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    AppConfigInterface& config_;
    ApplicationMetrics& systemMetrics_;

    uint16_t barWidth_;
    std::vector<uint16_t> previousBarHeights_;
    unsigned long lastDataUpdateTimestamp_ = 0;

    std::unique_ptr<ValueSmoother> valueSmoother_;
    std::vector<uint8_t> smoothedThreadLoads_;

    void drawBars();
    void updateSmoothedValues();
};