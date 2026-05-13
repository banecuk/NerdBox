#pragma once

#include "ui/widgets/base/Widget.h"
#include "utils/ApplicationMetrics.h"

// Displays device uptime as HH:MM:SS, updating once per second.
// Styled to match IpAddressWidget: a small grey label above a larger value.
class UptimeWidget : public Widget {
 public:
    UptimeWidget(const WidgetInterface::Dimensions& dims,
                 ApplicationMetrics& systemMetrics,
                 uint16_t textColor = TFT_GREEN,
                 uint16_t bgColor   = TFT_BLACK);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    ApplicationMetrics& systemMetrics_;
    uint16_t textColor_;
    uint16_t bgColor_;

    // Last rendered string — avoids a redraw when the second hasn't ticked.
    char lastRendered_[9] = {};  // "HH:MM:SS" + null
};
