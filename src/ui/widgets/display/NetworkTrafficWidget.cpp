#include "NetworkTrafficWidget.h"

#include <cstdio>

#include "config/Environment.h"
#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

NetworkTrafficWidget::NetworkTrafficWidget(const WidgetInterface::Dimensions& dims,
                                           uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.is_available, pcMetrics.last_update_timestamp) {}

void NetworkTrafficWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Reset cache so onDraw does a full repaint.
    lastUpMbps_ = -1.0f;
    lastDownMbps_ = -1.0f;
    lastHasData_ = false;
}

void NetworkTrafficWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_)
        return;

    const bool hasData = freshnessGuard_.isFresh();
    const float upMbps = hasData ? pcMetrics_.eth_up * kKBpsToMbps : 0.0f;
    const float downMbps = hasData ? pcMetrics_.eth_dn * kKBpsToMbps : 0.0f;

    const bool changed = forceRedraw || hasData != lastHasData_ || upMbps != lastUpMbps_ ||
                         downMbps != lastDownMbps_;
    if (!changed)
        return;

    const int16_t rowH = dimensions_.height / 2;
    drawRow(dimensions_.y, true, upMbps, ETH_UPLOAD_MBPS, hasData);
    drawRow(dimensions_.y + rowH, false, downMbps, ETH_DOWNLOAD_MBPS, hasData);

    lastUpMbps_ = upMbps;
    lastDownMbps_ = downMbps;
    lastHasData_ = hasData;
}

void NetworkTrafficWidget::drawRow(int16_t rowY, bool isUp, float mbps, float maxMbps,
                                   bool hasData) {
    LGFX* lcd = getLcd();
    const int16_t rowH = dimensions_.height / 2;

    lcd->fillRect(dimensions_.x, rowY, dimensions_.width, rowH, TFT_BLACK);

    const float percent = (hasData && maxMbps > 0.0f) ? (mbps / maxMbps) * 100.0f : 0.0f;
    const uint16_t color = hasData ? trafficColor(mbps, percent) : Colors::kHairline;

    const int16_t iconCx = dimensions_.x + 9;
    const int16_t iconCy = rowY + rowH / 2;
    drawArrow(iconCx, iconCy, isUp, color);

    // "M" suffix instead of "Mbps" — keeps the string short enough for the
    // larger value font to have room to breathe in the remaining width.
    char buf[16];
    if (!hasData) {
        snprintf(buf, sizeof(buf), "--");
    } else if (mbps < 10.0f) {
        snprintf(buf, sizeof(buf), "%.1fM", static_cast<double>(mbps));
    } else {
        snprintf(buf, sizeof(buf), "%dM", static_cast<int>(mbps + 0.5f));
    }

    Fonts::loadValue(lcd);
    lcd->setTextColor(color, TFT_BLACK);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(buf, dimensions_.x + 20, iconCy);
    Fonts::unload(lcd);
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
        return Colors::kInactiveText;  // idle
    if (percent < 60.0f)
        return TFT_GREEN;  // light/moderate
    if (percent < 85.0f)
        return TFT_YELLOW;  // heavy
    if (percent < 100.0f)
        return TFT_ORANGE;  // near saturation
    return Colors::blendRgb565(TFT_RED, TFT_WHITE, 90);  // at/over configured link speed
}

bool NetworkTrafficWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
