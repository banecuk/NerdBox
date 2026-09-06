#include "NetworkTrafficWidget.h"

#include "config/Environment.h"
#include "ui/core/Colors.h"
#include "ui/widgets/base/WidgetPainter.h"

NetworkTrafficWidget::NetworkTrafficWidget(const WidgetInterface::Dimensions& dims,
                                           uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics), freshnessGuard_(pcMetrics.freshness) {}

void NetworkTrafficWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Reset cache so onDraw does a full repaint.
    lastUpText_[0] = '\0';
    lastDownText_[0] = '\0';
    lastUpColor_ = 0;
    lastDownColor_ = 0;
    lastHasData_ = false;
}

void NetworkTrafficWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_)
        return;

    const bool hasData = freshnessGuard_.isFresh();
    const float upMbps = hasData ? pcMetrics_.eth_up * kKBpsToMbps : 0.0f;
    const float downMbps = hasData ? pcMetrics_.eth_dn * kKBpsToMbps : 0.0f;

    const bool dataAvailabilityChanged = forceRedraw || hasData != lastHasData_;
    const int16_t rowH = dimensions_.height / 2;
    drawRow(dimensions_.y, true, upMbps, ETH_UPLOAD_MBPS, hasData, dataAvailabilityChanged,
            lastUpText_, sizeof(lastUpText_), lastUpColor_);
    drawRow(dimensions_.y + rowH, false, downMbps, ETH_DOWNLOAD_MBPS, hasData,
            dataAvailabilityChanged, lastDownText_, sizeof(lastDownText_), lastDownColor_);

    lastHasData_ = hasData;
}

void NetworkTrafficWidget::drawRow(int16_t rowY, bool isUp, float mbps, float maxMbps, bool hasData,
                                   bool forceRedraw, char* lastText, size_t lastTextSize,
                                   uint16_t& lastColor) {
    const float percent = (hasData && maxMbps > 0.0f) ? (mbps / maxMbps) * 100.0f : 0.0f;
    const uint16_t color = hasData ? trafficColor(mbps, percent) : Colors::kHairline;

    const int16_t rowH = dimensions_.height / 2;
    const WidgetPainter::RateRow row{static_cast<int16_t>(dimensions_.x), rowY,
                                     static_cast<int16_t>(dimensions_.width), rowH,
                                     mbps, hasData, /*intDigits=*/3, color,
                                     lastText, lastTextSize, &lastColor, forceRedraw};
    const int16_t suffixX = WidgetPainter::drawRateRow(getLcd(), row);
    if (suffixX < 0)
        return;  // rendered text+colour unchanged — arrow stays as-is too

    // Arrow sits right after the (fixed-width) value field, not in front of it.
    const int16_t iconCy = rowY + rowH / 2;
    drawArrow(suffixX + 10, iconCy, isUp, color);
}

void NetworkTrafficWidget::drawArrow(int16_t cx, int16_t cy, bool up, uint16_t color) {
    LGFX* lcd = getLcd();
    constexpr int8_t kHalfW = 5;
    constexpr int8_t kHalfH = 4;

    if (up) {
        lcd->fillTriangle(cx, cy - kHalfH, cx - kHalfW, cy + kHalfH, cx + kHalfW, cy + kHalfH,
                          color);
    } else {
        lcd->fillTriangle(cx, cy + kHalfH, cx - kHalfW, cy - kHalfH, cx + kHalfW, cy - kHalfH,
                          color);
    }
}

uint16_t NetworkTrafficWidget::trafficColor(float mbps, float percent) {
    if (mbps < 0.2f)
        return getContext().getColors().getColorFromPercentGrayGreen(0);  // idle
    return getContext().getColors().utilizationColor(percent);
}

bool NetworkTrafficWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
