#pragma once

#include "ui/resources/FontRegistry.h"
#include "ui/widgets/base/HmsFieldRenderer.h"
#include "ui/widgets/base/Widget.h"
#include "utils/ApplicationMetrics.h"

// Displays device uptime as HH:MM:SS, updating once per second. Past 99h
// uptime, ApplicationMetrics switches the source string to "Dd HH:MM" — this
// widget falls back to a plain whole-string redraw in that mode since the
// fixed-offset per-field diffing below assumes exactly 2-digit HH.
// Styled to match IpAddressWidget: a small grey label above a larger value.
class UptimeWidget : public Widget {
 public:
    UptimeWidget(const WidgetInterface::Dimensions& dims, ApplicationMetrics& systemMetrics,
                 uint16_t textColor = TFT_GREEN, uint16_t bgColor = TFT_BLACK);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    void computeLayout();

    ApplicationMetrics& systemMetrics_;
    uint16_t textColor_;
    uint16_t bgColor_;

    // Last rendered string — avoids a redraw when the second hasn't ticked.
    // Sized for the "Dd HH:MM" fallback format (days up to 5 digits), not
    // just "HH:MM:SS" + null.
    char lastRendered_[16] = {};
    bool dayMode_ = false;  // true once ApplicationMetrics switches formats

    // Font-measured layout — computed once on first drawStatic().
    bool layoutReady_ = false;
    uint16_t valueY_ = 0;  // y of the value row
    uint16_t digitW_ = 0;  // pixel width of "00" in NotoSansDisplay15
    uint16_t colonW_ = 0;  // pixel width of ":"
    uint16_t xHH_ = 0;
    uint16_t xColon1_ = 0;
    uint16_t xColon2_ = 0;

    HmsFieldRenderer fieldRenderer_;
};
