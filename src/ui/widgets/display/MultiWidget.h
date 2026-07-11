#pragma once

#include "ui/widgets/base/Widget.h"

// Multifunctional content area on MainScreen — sits below the AirQualityWidget,
// left of the FpsWidget, and above the ClockWidget / NetworkWidget row.
// Placeholder: draws only a 1px border. Will later switch its content
// dynamically based on which data is most relevant (e.g. CPU load, weather).
class MultiWidget : public Widget {
 public:
    MultiWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint16_t kBorderColor = 0x2104;  // dark grey, matches other widgets
};
