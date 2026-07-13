#pragma once

#include <atomic>

#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Compact Ethernet throughput widget — sits right of the Settings button in
// the bottom band (48 x 48). Two stacked rows: upload on top, download on
// bottom, each with a triangle direction icon and a "NM" (Mbps) value in the
// 15pt value font. Colour reflects utilisation against the configured link
// speed (Environment.h's ETH_UPLOAD_MBPS / ETH_DOWNLOAD_MBPS) — grey below
// 0.2 Mbps, green/yellow/orange as the link fills up, a lightened red once
// at or over the configured capacity.
//
// PcMetrics::eth_up/eth_dn arrive in KB/s (1 KB = 1024 bytes); this widget
// converts to Mbps for display and thresholding.
class NetworkTrafficWidget : public Widget {
 public:
    NetworkTrafficWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    PcMetrics& pcMetrics_;
    DataFreshnessGuard<std::atomic<bool>, unsigned long> freshnessGuard_;

    // Cached state for dirty detection — sentinels force the first draw.
    float lastUpMbps_ = -1.0f;
    float lastDownMbps_ = -1.0f;
    bool lastHasData_ = false;

    void drawRow(int16_t rowY, bool isUp, float mbps, float maxMbps, bool hasData);
    void drawArrow(int16_t cx, int16_t cy, bool up, uint16_t color);

    // Colour by absolute Mbps (idle threshold) and utilisation percentage of
    // the configured link speed (higher tiers).
    static uint16_t trafficColor(float mbps, float percent);

    // KB/s (1024 bytes) -> Mbps.
    static constexpr float kKBpsToMbps = (1024.0f * 8.0f) / 1000000.0f;
};
