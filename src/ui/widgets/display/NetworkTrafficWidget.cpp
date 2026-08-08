#include "NetworkTrafficWidget.h"

#include <cstdio>
#include <cstring>

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

void NetworkTrafficWidget::drawRow(int16_t rowY, bool isUp, float mbps, float maxMbps,
                                   bool hasData, bool forceRedraw, char* lastText,
                                   size_t lastTextSize, uint16_t& lastColor) {
    const float percent = (hasData && maxMbps > 0.0f) ? (mbps / maxMbps) * 100.0f : 0.0f;
    const uint16_t color = hasData ? trafficColor(mbps, percent) : Colors::kHairline;

    // No unit suffix, fixed "123.4" layout (3 integer digits reserved, right-
    // aligned, one decimal) so the value never shifts width as it grows —
    // paired with the monospace font below for stable column alignment. Also
    // matches the displayed precision so the redraw check below is comparing
    // what's actually on screen, not the noisy raw float.
    char buf[16];
    if (!hasData) {
        snprintf(buf, sizeof(buf), "  --");
    } else {
        snprintf(buf, sizeof(buf), "%5.1f", static_cast<double>(mbps));
    }

    if (!forceRedraw && color == lastColor && strncmp(buf, lastText, lastTextSize) == 0)
        return;

    LGFX* lcd = getLcd();
    const int16_t rowH = dimensions_.height / 2;

    lcd->fillRect(dimensions_.x, rowY, dimensions_.width, rowH, TFT_BLACK);

    const int16_t iconCy = rowY + rowH / 2;
    const int16_t textX = dimensions_.x + 2;

    Fonts::loadMono(lcd);
    lcd->setTextColor(color, TFT_BLACK);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(buf, textX, iconCy);
    const int16_t textWidth = lcd->textWidth(buf);
    Fonts::unload(lcd);

    // Arrow sits right after the value, not in front of it.
    const int16_t iconCx = textX + textWidth + 10;
    drawArrow(iconCx, iconCy, isUp, color);

    strncpy(lastText, buf, lastTextSize - 1);
    lastText[lastTextSize - 1] = '\0';
    lastColor = color;
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
        return TFT_LIGHTGREY;  // idle — kInactiveText is too dark to read at this size
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
