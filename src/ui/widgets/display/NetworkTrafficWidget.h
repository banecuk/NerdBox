#pragma once

#include <atomic>
#include <cstddef>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "utils/CachedString.h"
#include "utils/FreshnessTransition.h"

// Compact Ethernet throughput widget — sits right of the Settings button in
// the bottom band. Two stacked rows: upload on top, download on bottom, each
// with a triangle direction icon and a bare Mbps value (no unit suffix, one
// decimal). The integer part is right-aligned to a fixed column sized for up
// to 3 digits ("999.9") and the decimal suffix is drawn from that same
// column, so the decimal point stays in the same place regardless of how
// many integer digits the value has, and the arrow position never depends on
// the rendered string's measured width. Colour reflects utilisation against
// the configured link speed (AppSettings::endpoints.ethUploadMbps /
// ethDownloadMbps, from Environment.h via the screen's config) — a smooth light-grey-to-light-green
// ramp (via Colors::getColorFromPercentGrayGreen(), the same precomputed table CpuClockWidget uses)
// from idle up to 60% utilisation, then yellow/orange as the link fills further, a lightened red
// once at or over the configured capacity.
//
// PcMetrics::eth_up/eth_dn arrive in KB/s (1 KB = 1024 bytes); this widget
// converts to Mbps for display and thresholding.
class NetworkTrafficWidget : public Widget {
 public:
    NetworkTrafficWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         PcMetrics& pcMetrics, float uploadLinkMbps, float downloadLinkMbps);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    PcMetrics& pcMetrics_;
    FreshnessTransition freshness_;
    const float uploadLinkMbps_;
    const float downloadLinkMbps_;

    // Cached *rendered* state (formatted text + colour actually drawn last
    // time), not the raw Mbps value — comparing raw floats redraws on every
    // tiny fluctuation even when the rounded text on screen wouldn't change,
    // which reads as the units flickering. Empty string forces the first draw.
    CachedString<16> lastUpText_;
    CachedString<16> lastDownText_;
    uint16_t lastUpColor_ = 0;
    uint16_t lastDownColor_ = 0;

    void drawRow(int16_t rowY, bool isUp, float mbps, float maxMbps, bool hasData, bool forceRedraw,
                 CachedString<16>& lastText, uint16_t& lastColor);
    void drawArrow(int16_t cx, int16_t cy, bool up, uint16_t color);

    // Colour by absolute Mbps (idle threshold) and utilisation percentage of
    // the configured link speed (higher tiers). Not static: the idle/
    // low-utilisation range indexes Colors::getColorFromPercentGrayGreen()'s
    // precomputed table via getContext(), same pattern as CpuClockWidget.
    uint16_t trafficColor(float mbps, float percent);

    // KB/s (1024 bytes) -> Mbps.
    static constexpr float kKBpsToMbps = (1024.0f * 8.0f) / 1000000.0f;
};
