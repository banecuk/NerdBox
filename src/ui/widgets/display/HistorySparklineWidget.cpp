#include "HistorySparklineWidget.h"

#include "core/resources/FontRegistry.h"

static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kCpuColor = 0xC618;
static constexpr uint16_t kGpuColor = 0xB471;
static constexpr uint8_t kLowUsagePercent = 20;

// Halves RGB565 brightness while respecting the 5-6-5 channel split — shift
// every bit right by one, then mask off the bit that would otherwise leak
// across a channel boundary (0x7BEF = 0111 1011 1110 1111).
static constexpr uint16_t dimColor(uint16_t color) {
    return (color >> 1) & 0x7BEF;
}
static constexpr uint16_t kCpuColorDim = dimColor(kCpuColor);
static constexpr uint16_t kGpuColorDim = dimColor(kGpuColor);

// Encodes a column's plotted state (bar height + below-threshold flag) into
// a single byte so drawRow's redraw-skip check catches a color-only change
// (crossing the low-usage threshold at the same pixel height) too.
static constexpr uint8_t kLowUsageFlag = 0x80;

HistorySparklineWidget::HistorySparklineWidget(const WidgetInterface::Dimensions& dims,
                                               uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.freshness),
      cpuLoadHistory_(kHistorySize),
      gpuLoadHistory_(kHistorySize) {}

void HistorySparklineWidget::computeLayout() {
    plotWidth_ = kHistorySize * kColWidth;
    const uint16_t maxWidth = dimensions_.width - kCaptionWidth;
    if (plotWidth_ > maxWidth)
        plotWidth_ = maxWidth;

    // Center the caption+plot block horizontally — plotWidth_ (fixed at
    // kHistorySize columns) is normally narrower than the widget, so without
    // this the row reads as flush-left with dead space on the right.
    const uint16_t contentWidth = kCaptionWidth + plotWidth_;
    const uint16_t hOffset = (dimensions_.width - contentWidth) / 2;
    captionX_ = dimensions_.x + hOffset;
    plotX_ = captionX_ + kCaptionWidth;

    rowHeight_ = (dimensions_.height - 2 * kRowMargin - kRowGap) / 2;
    cpuPlotY_ = dimensions_.y + kRowMargin;
    gpuPlotY_ = cpuPlotY_ + rowHeight_ + kRowGap;
}

void HistorySparklineWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    computeLayout();

    // Vertically centered on each row so the label reads level with the
    // sparkline beside it, not pinned to the row's top edge.
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString("CPU", captionX_, cpuPlotY_ + rowHeight_ / 2);
    lcd->drawString("GPU", captionX_, gpuPlotY_ + rowHeight_ / 2);
    Fonts::unload(lcd);

    for (auto& h : lastCpuCol_)
        h = 0;
    for (auto& h : lastGpuCol_)
        h = 0;
}

void HistorySparklineWidget::sampleIfNeeded() {
    if (!freshnessGuard_.isFresh())
        return;
    const uint32_t now = millis();
    if (now - lastSampleMs_ < kSampleIntervalMs)
        return;

    lastSampleMs_ = now;
    cpuLoadHistory_.push(pcMetrics_.cpu_load);
    gpuLoadHistory_.push(pcMetrics_.gpu_load);
}

void HistorySparklineWidget::drawRow(uint16_t plotY, const PsramRingHistory<uint8_t>& history,
                                     uint8_t* lastCol, uint16_t color, uint16_t lowColor,
                                     bool forceFullRepaint) {
    LGFX* lcd = getLcd();
    const size_t count = history.size();
    const size_t offset = kHistorySize - count;

    for (size_t col = 0; col < kHistorySize; ++col) {
        uint8_t height = 0;
        bool lowUsage = false;
        if (col >= offset) {
            const size_t idx = col - offset;
            const uint8_t rawValue = history.at(idx);
            const uint32_t h =
                (static_cast<uint32_t>(rawValue) * (rowHeight_ - 1)) / kScaleMax;
            height = static_cast<uint8_t>((h > rowHeight_ - 1 ? rowHeight_ - 1 : h) + 1);
            lowUsage = rawValue < kLowUsagePercent;
        }

        const uint8_t encoded = height | (lowUsage ? kLowUsageFlag : 0);
        if (!forceFullRepaint && encoded == lastCol[col])
            continue;

        const uint16_t x = plotX_ + static_cast<uint16_t>(col) * kColWidth;

        lcd->fillRect(x, plotY, kColWidth, rowHeight_, kBgColor);
        if (height > 0)
            lcd->fillRect(x, plotY + rowHeight_ - height, kColWidth, 1,
                         lowUsage ? lowColor : color);

        lastCol[col] = encoded;
    }
}

void HistorySparklineWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    sampleIfNeeded();

    LGFX* lcd = getLcd();
    lcd->startWrite();
    drawRow(cpuPlotY_, cpuLoadHistory_, lastCpuCol_, kCpuColor, kCpuColorDim, forceRedraw);
    drawRow(gpuPlotY_, gpuLoadHistory_, lastGpuCol_, kGpuColor, kGpuColorDim, forceRedraw);
    lcd->endWrite();

    clearDirty();
}
