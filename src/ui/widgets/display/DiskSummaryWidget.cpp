#include "DiskSummaryWidget.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"
#include "utils/ScopedLock.h"

DiskSummaryWidget::DiskSummaryWidget(const WidgetInterface::Dimensions& dims,
                                     uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                                     EventType action, ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.freshness),
      action_(action),
      callback_(std::move(callback)) {}

void DiskSummaryWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Reset cache so onDraw does a full repaint.
    lastReadText_[0] = '\0';
    lastWriteText_[0] = '\0';
    lastReadColor_ = 0;
    lastWriteColor_ = 0;
    lastHasData_ = false;
}

void DiskSummaryWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !isStaticDrawn_)
        return;

    const bool hasData = freshnessGuard_.isFresh();

    float readKBps = 0.0f;
    float writeKBps = 0.0f;
    if (hasData) {
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        for (const auto& drive : pcMetrics_.disk_drives) {
            readKBps += drive.readKBPerSec;
            writeKBps += drive.writeKBPerSec;
        }
    }

    const bool dataAvailabilityChanged = forceRedraw || hasData != lastHasData_;
    const int16_t rowH = dimensions_.height / 2;
    drawRow(dimensions_.y, false, writeKBps * kKBpsToMBps, hasData, dataAvailabilityChanged,
            lastWriteText_, sizeof(lastWriteText_), lastWriteColor_);
    drawRow(dimensions_.y + rowH, true, readKBps * kKBpsToMBps, hasData, dataAvailabilityChanged,
            lastReadText_, sizeof(lastReadText_), lastReadColor_);

    lastHasData_ = hasData;
}

void DiskSummaryWidget::drawRow(int16_t rowY, bool isRead, float mbps, bool hasData,
                                bool forceRedraw, char* lastText, size_t lastTextSize,
                                uint16_t& lastColor) {
    // Colors::disk*ActivityColor() render idle (<1 MB/s) as a dark hairline
    // grey, meant to disappear against DiskBandWidget's black background.
    // Here — like NetworkTrafficWidget's idle state — no activity should
    // still read clearly against the row's black background, so idle uses
    // the same light-grey ramp floor (getColorFromPercentGrayGreen(0)) as
    // NetworkTrafficWidget instead.
    constexpr float kIdleMBps = 1.0f;
    uint16_t color = Colors::kHairline;
    if (hasData) {
        color = (mbps < kIdleMBps)
                    ? getContext().getColors().getColorFromPercentGrayGreen(0)
                    : (isRead ? Colors::diskReadActivityColor(mbps * 1024.0f) : writeColor(mbps));
    }

    // Split into integer and decimal parts, same rationale as
    // NetworkTrafficWidget::drawRow: the letter suffix's x position must not
    // depend on the value string's measured width, which right-justified
    // space-padding can't guarantee across fonts.
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

    const int16_t textY = rowY + rowH / 2;
    const int16_t textX = dimensions_.x + 2;

    Fonts::loadValue(lcd);
    lcd->setTextColor(color, TFT_BLACK);

    // Right-align the integer part to a fixed column (up to 4 digits, e.g.
    // "9999" — the summed rate across drives can comfortably exceed a single
    // NVMe drive's rate) so it lands in the same place regardless of digit
    // count, then draw the decimal suffix left-aligned from that same
    // column, same as NetworkTrafficWidget.
    const int16_t intFieldWidth = lcd->textWidth("9999");
    const int16_t decFieldWidth = lcd->textWidth(".9");
    const int16_t intColX = textX + intFieldWidth;

    lcd->setTextDatum(MR_DATUM);
    lcd->drawString(intBuf, intColX, textY);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(decBuf, intColX, textY);

    // Letter suffix sits right after the (fixed-width) value field, at a
    // position that never depends on the value string's measured width.
    // Colored independently of the value digits above: any nonzero rate
    // lights it up green (read) or red (write) — dim (dark green/TFT_MAROON,
    // the correct dark-red anchor per Colors::diskWriteActivityColor's own
    // comment) below 1 MB/s, full brightness at/above it — matching
    // DiskBandWidget's activity-line colours. Unlike
    // Colors::diskReadActivityColor()/diskWriteActivityColor(), which treat
    // anything under 1 MB/s as fully idle, this must react to the smallest
    // detectable activity rather than only lighting up at 1 MB/s.
    constexpr float kFullBrightnessMBps = 1.0f;
    uint16_t letterColor = Colors::kHairline;
    if (hasData && mbps > 0.0f) {
        const bool dim = mbps < kFullBrightnessMBps;
        letterColor = isRead ? (dim ? TFT_DARKGREEN : TFT_GREEN) : (dim ? TFT_MAROON : TFT_RED);
    }
    const int16_t letterX = intColX + decFieldWidth + 6;
    lcd->setTextColor(letterColor, TFT_BLACK);
    lcd->drawString(isRead ? "R" : "W", letterX, textY);
    Fonts::unload(lcd);

    strncpy(lastText, buf, lastTextSize - 1);
    lastText[lastTextSize - 1] = '\0';
    lastColor = color;
}

uint16_t DiskSummaryWidget::writeColor(float mbps) {
    const float percent = (mbps / kWriteCapMBps) * 100.0f;
    if (percent < 60.0f) {
        const uint8_t idx = static_cast<uint8_t>(percent / 60.0f * 99.0f + 0.5f);
        return getContext().getColors().getColorFromPercentGrayGreen(idx);
    }
    if (percent < 85.0f)
        return TFT_YELLOW;
    if (percent < 100.0f)
        return TFT_ORANGE;
    return Colors::blendRgb565(TFT_RED, TFT_WHITE, 90);  // at/over the cap
}

bool DiskSummaryWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    if (!callback_)
        return false;
    callback_(action_);
    return true;
}
