#include "NetworkTrafficWidget.h"

#include <cstdio>
#include <cstring>

#include "config/Environment.h"
#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"

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

    // Split into integer and decimal parts so they can be drawn as two
    // separately-anchored strings (see below) instead of one space-padded
    // string. Padding with leading spaces to right-justify only works if the
    // font's space glyph has exactly a digit's advance width, which isn't
    // guaranteed — when it doesn't, the decimal point drifts between rows
    // and the arrow (positioned from the padded string's measured width)
    // drifts with it.
    char intBuf[16];
    char decBuf[4];
    if (!hasData) {
        snprintf(intBuf, sizeof(intBuf), "--");
        decBuf[0] = '\0';
    } else {
        char full[16];
        snprintf(full, sizeof(full), "%.1f", static_cast<double>(mbps));
        char* dot = strchr(full, '.');
        if (dot) {
            *dot = '\0';
            snprintf(intBuf, sizeof(intBuf), "%s", full);
            snprintf(decBuf, sizeof(decBuf), ".%s", dot + 1);
        } else {
            snprintf(intBuf, sizeof(intBuf), "%s", full);
            decBuf[0] = '\0';
        }
    }

    char buf[20];
    snprintf(buf, sizeof(buf), "%s%s", intBuf, decBuf);

    if (!forceRedraw && color == lastColor && strncmp(buf, lastText, lastTextSize) == 0)
        return;

    LGFX* lcd = getLcd();
    const int16_t rowH = dimensions_.height / 2;

    lcd->fillRect(dimensions_.x, rowY, dimensions_.width, rowH, TFT_BLACK);

    const int16_t iconCy = rowY + rowH / 2;
    const int16_t textX = dimensions_.x + 2;

    Fonts::loadValue(lcd);
    lcd->setTextColor(color, TFT_BLACK);

    // Right-align the integer part to a fixed column (up to 3 digits, e.g.
    // "999") so it lands in the same place regardless of how many digits the
    // value has, then draw the decimal suffix left-aligned from that same
    // column — the decimal point always lands in the same pixel column
    // whether the row reads "3.3" or "133.3".
    const int16_t intFieldWidth = lcd->textWidth("999");
    const int16_t decFieldWidth = lcd->textWidth(".9");
    const int16_t intColX = textX + intFieldWidth;

    lcd->setTextDatum(MR_DATUM);
    lcd->drawString(intBuf, intColX, iconCy);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(decBuf, intColX, iconCy);
    Fonts::unload(lcd);

    // Arrow sits right after the (fixed-width) value field, not in front of it.
    const int16_t iconCx = intColX + decFieldWidth + 10;
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
        return getContext().getColors().getColorFromPercentGrayGreen(0);  // idle
    if (percent < 60.0f) {
        // Smooth light-grey-to-light-green ramp across the idle-to-moderate
        // range, instead of an instant jump straight to full green the
        // moment traffic ticks up from idle.
        const uint8_t idx = static_cast<uint8_t>(percent / 60.0f * 99.0f + 0.5f);
        return getContext().getColors().getColorFromPercentGrayGreen(idx);
    }
    if (percent < 85.0f)
        return TFT_YELLOW;  // heavy
    if (percent < 100.0f)
        return TFT_ORANGE;                               // near saturation
    return Colors::blendRgb565(TFT_RED, TFT_WHITE, 90);  // at/over configured link speed
}

bool NetworkTrafficWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
