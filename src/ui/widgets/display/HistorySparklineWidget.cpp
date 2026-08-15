#include "HistorySparklineWidget.h"

#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"

static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kCpuColor = 0xC618;
static constexpr uint16_t kGpuColor = 0xB471;
static constexpr uint16_t kRamColor = 0xADFB;
static constexpr uint16_t kVramColor = kGpuColor;  // same GPU accent used in PcMetricsTilesConfig
static constexpr uint16_t kHighUsageColor = TFT_RED;  // shared alert color, any metric >= 90%
// Very dark gray divider drawn above/below each of the 4 sparkline rows.
static constexpr uint16_t kRowBorderColor = 0x18C3;

// Warning-band colors: a blend of each metric's normal color and the alert
// color, so the transition into the red zone reads as a ramp instead of a
// sudden jump. Dynamic init (not constexpr) since Colors::blendRgb565 isn't
// a constexpr function, but these still run before setup().
static const uint16_t kCpuWarnColor = Colors::blendRgb565(kCpuColor, kHighUsageColor, 128);
static const uint16_t kGpuWarnColor = Colors::blendRgb565(kGpuColor, kHighUsageColor, 128);
static const uint16_t kRamWarnColor = Colors::blendRgb565(kRamColor, kHighUsageColor, 128);
static const uint16_t kVramWarnColor = Colors::blendRgb565(kVramColor, kHighUsageColor, 128);

// Halves RGB565 brightness while respecting the 5-6-5 channel split — shift
// every bit right by one, then mask off the bit that would otherwise leak
// across a channel boundary (0x7BEF = 0111 1011 1110 1111).
static constexpr uint16_t dimColor(uint16_t color) {
    return (color >> 1) & 0x7BEF;
}
static constexpr uint16_t kCpuColorDim = dimColor(kCpuColor);
static constexpr uint16_t kGpuColorDim = dimColor(kGpuColor);
static constexpr uint16_t kRamColorDim = dimColor(kRamColor);
static constexpr uint16_t kVramColorDim = dimColor(kVramColor);

// Encodes a column's plotted state (bar height + usage zone) into a single
// byte so drawRow's redraw-skip check catches a color-only change (crossing
// a threshold at the same pixel height) too. Zone occupies the top 2 bits
// (4 mutually-exclusive states), height the bottom 6.
static constexpr uint8_t kZoneNormal = 0;
static constexpr uint8_t kZoneLow = 1;
static constexpr uint8_t kZoneWarn = 2;
static constexpr uint8_t kZoneHigh = 3;
static constexpr uint8_t kZoneShift = 6;
static constexpr uint8_t kHeightMask = 0x3F;

uint16_t HistorySparklineWidget::plotWidthForHalf(const WidgetInterface::Dimensions& dims,
                                                  bool leftHalf) {
    const uint16_t halfWidth = dims.width / 2;
    const uint16_t sectionWidth = leftHalf ? halfWidth : (dims.width - halfWidth);
    return sectionWidth > kCaptionWidth ? sectionWidth - kCaptionWidth : 0;
}

HistorySparklineWidget::HistorySparklineWidget(const WidgetInterface::Dimensions& dims,
                                               uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.freshness),
      cpuLoadHistory_(plotWidthForHalf(dims, true) / kColWidth),
      gpuLoadHistory_(plotWidthForHalf(dims, true) / kColWidth),
      ramLoadHistory_(plotWidthForHalf(dims, false) / kColWidth),
      vramLoadHistory_(plotWidthForHalf(dims, false) / kColWidth) {}

void HistorySparklineWidget::computeLayout() {
    const uint16_t halfWidth = dimensions_.width / 2;

    leftCaptionX_ = dimensions_.x;
    leftPlotX_ = leftCaptionX_ + kCaptionWidth;
    leftPlotWidth_ = plotWidthForHalf(dimensions_, true);
    leftCols_ = leftPlotWidth_ / kColWidth;
    if (leftCols_ > kMaxColumns)
        leftCols_ = kMaxColumns;

    rightCaptionX_ = dimensions_.x + halfWidth;
    rightPlotX_ = rightCaptionX_ + kCaptionWidth;
    rightPlotWidth_ = plotWidthForHalf(dimensions_, false);
    rightCols_ = rightPlotWidth_ / kColWidth;
    if (rightCols_ > kMaxColumns)
        rightCols_ = kMaxColumns;

    rowHeight_ = (dimensions_.height - 2 * kRowMargin - kRowGap) / 2;
    topPlotY_ = dimensions_.y + kRowMargin;
    bottomPlotY_ = topPlotY_ + rowHeight_ + kRowGap;
}

void HistorySparklineWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    computeLayout();

    // Centered in each caption column (kCaptionWidth wide) and vertically
    // centered on its row so the label reads level with the sparkline beside
    // it, not pinned to the row's top edge.
    const uint16_t leftCaptionCenterX = leftCaptionX_ + kCaptionWidth / 2;
    const uint16_t rightCaptionCenterX = rightCaptionX_ + kCaptionWidth / 2;

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("CPU", leftCaptionCenterX, topPlotY_ + rowHeight_ / 2);
    lcd->drawString("GPU", leftCaptionCenterX, bottomPlotY_ + rowHeight_ / 2);
    lcd->drawString("RAM", rightCaptionCenterX, topPlotY_ + rowHeight_ / 2);
    lcd->drawString("VRM", rightCaptionCenterX, bottomPlotY_ + rowHeight_ / 2);
    Fonts::unload(lcd);

    drawRowBorders();

    for (auto& h : lastCpuCol_)
        h = 0;
    for (auto& h : lastGpuCol_)
        h = 0;
    for (auto& h : lastRamCol_)
        h = 0;
    for (auto& h : lastVramCol_)
        h = 0;
}

void HistorySparklineWidget::drawRowBorders() {
    LGFX* lcd = getLcd();
    // One line above the top row, one shared divider between the two rows
    // (the 1px kRowGap), and one line below the bottom row — tucked into the
    // margin/gap pixels reserved by kRowMargin/kRowGap so they never overlap
    // the plotted data.
    const uint16_t topBorder = topPlotY_ - 1;
    const uint16_t middleDivider = topPlotY_ + rowHeight_;
    const uint16_t bottomBorder = bottomPlotY_ + rowHeight_;

    lcd->drawFastHLine(leftPlotX_, topBorder, leftPlotWidth_, kRowBorderColor);
    lcd->drawFastHLine(leftPlotX_, middleDivider, leftPlotWidth_, kRowBorderColor);
    lcd->drawFastHLine(leftPlotX_, bottomBorder, leftPlotWidth_, kRowBorderColor);

    lcd->drawFastHLine(rightPlotX_, topBorder, rightPlotWidth_, kRowBorderColor);
    lcd->drawFastHLine(rightPlotX_, middleDivider, rightPlotWidth_, kRowBorderColor);
    lcd->drawFastHLine(rightPlotX_, bottomBorder, rightPlotWidth_, kRowBorderColor);
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
    ramLoadHistory_.push(pcMetrics_.mem_load);
    // gpu_mem is a percent that can exceed 100 on some drivers; clamp to the
    // sparkline's fixed 0-100 scale.
    const uint16_t vram = pcMetrics_.gpu_mem;
    vramLoadHistory_.push(vram > kScaleMax ? kScaleMax : static_cast<uint8_t>(vram));
}

void HistorySparklineWidget::drawRow(uint16_t plotX, uint16_t plotY, uint16_t cols,
                                     const HeapRingHistory<uint8_t>& history, uint8_t* lastCol,
                                     uint16_t color, uint16_t lowColor, uint8_t lowThreshold,
                                     uint8_t warnThreshold, uint16_t warnColor,
                                     bool forceFullRepaint) {
    LGFX* lcd = getLcd();
    const size_t count = history.size();
    const size_t offset = cols > count ? cols - count : 0;
    // Computed once per row instead of once per column inside the loop below
    // — history.at(idx) would otherwise redo this same modulo on every call.
    const size_t start = history.startIndex();

    // Every new sample re-indexes the whole ring buffer by one slot, so a
    // volatile metric (CPU) typically needs nearly every column repainted on
    // each sample tick. Redrawing those column-by-column (clear + draw
    // interleaved) turns into hundreds of tiny SPI transfers that visibly
    // "sweep" across the row. Above a threshold, clear the whole row in one
    // transfer and draw the dots in a second pass instead — far fewer SPI
    // commands, so the row updates as a single atomic-looking repaint.
    static constexpr size_t kBatchThresholdCols = 8;

    uint8_t encodedCols[kMaxColumns];
    size_t changedCount = 0;

    for (size_t col = 0; col < cols; ++col) {
        uint8_t height = 0;
        uint8_t zone = kZoneNormal;
        if (col >= offset) {
            const size_t idx = col - offset;
            const uint8_t rawValue = history.valueAtOffset(start, idx);
            const uint32_t h = (static_cast<uint32_t>(rawValue) * (rowHeight_ - 1)) / kScaleMax;
            height = static_cast<uint8_t>((h > rowHeight_ - 1 ? rowHeight_ - 1 : h) + 1);
            if (rawValue < lowThreshold)
                zone = kZoneLow;
            else if (rawValue >= kHighUsageThreshold)
                zone = kZoneHigh;
            else if (warnThreshold != kNoWarnThreshold && rawValue >= warnThreshold)
                zone = kZoneWarn;
        }

        const uint8_t encoded = static_cast<uint8_t>((height & kHeightMask) | (zone << kZoneShift));
        encodedCols[col] = encoded;
        if (forceFullRepaint || encoded != lastCol[col])
            ++changedCount;
    }

    if (changedCount == 0)
        return;

    const bool batch = forceFullRepaint || changedCount > kBatchThresholdCols;
    if (batch)
        lcd->fillRect(plotX, plotY, static_cast<uint16_t>(cols) * kColWidth, rowHeight_, kBgColor);

    for (size_t col = 0; col < cols; ++col) {
        const uint8_t encoded = encodedCols[col];
        if (!batch && encoded == lastCol[col])
            continue;

        const uint16_t x = plotX + static_cast<uint16_t>(col) * kColWidth;
        const uint8_t height = encoded & kHeightMask;
        const uint8_t zone = encoded >> kZoneShift;

        if (!batch)
            lcd->fillRect(x, plotY, kColWidth, rowHeight_, kBgColor);
        if (height > 0) {
            uint16_t barColor = color;
            if (zone == kZoneHigh)
                barColor = kHighUsageColor;
            else if (zone == kZoneWarn)
                barColor = warnColor;
            else if (zone == kZoneLow)
                barColor = lowColor;

            // Draw a kSparkThickness-tall marker instead of a single pixel,
            // clamped so it never overflows below the row (the min-height
            // case) — the row's bottom edge is the border line drawn in
            // drawRowBorders(). Below the low threshold, thin it to 1px so
            // quiet/idle stretches read as visually lighter than active ones.
            const uint16_t thickness = zone == kZoneLow ? 1 : kSparkThickness;
            uint16_t barY = plotY + rowHeight_ - height;
            if (barY + thickness > plotY + rowHeight_)
                barY = plotY + rowHeight_ - thickness;
            lcd->fillRect(x, barY, kColWidth, thickness, barColor);
        }

        lastCol[col] = encoded;
    }
}

void HistorySparklineWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool hasData = freshnessGuard_.isFresh();
    if (!hasData) {
        // Blank the whole widget (captions included) the moment data goes
        // stale, rather than leaving a frozen last-known plot on screen —
        // an unmoving sparkline reads as live data, which it no longer is.
        if (forceRedraw || lastHasData_) {
            getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                               kBgColor);
        }
        lastHasData_ = false;
        clearDirty();
        return;
    }

    // Coming back from stale: captions were blanked above, so repaint them
    // (and reset the redraw caches so every column repaints fresh) before
    // resuming normal per-column diffing.
    if (!lastHasData_) {
        onDrawStatic();
        forceRedraw = true;
    }
    lastHasData_ = true;

    sampleIfNeeded();

    LGFX* lcd = getLcd();
    lcd->startWrite();
    drawRow(leftPlotX_, topPlotY_, leftCols_, cpuLoadHistory_, lastCpuCol_, kCpuColor, kCpuColorDim,
            kCpuGpuLowThreshold, kCpuGpuWarnThreshold, kCpuWarnColor, forceRedraw);
    drawRow(leftPlotX_, bottomPlotY_, leftCols_, gpuLoadHistory_, lastGpuCol_, kGpuColor,
            kGpuColorDim, kCpuGpuLowThreshold, kCpuGpuWarnThreshold, kGpuWarnColor, forceRedraw);
    drawRow(rightPlotX_, topPlotY_, rightCols_, ramLoadHistory_, lastRamCol_, kRamColor,
            kRamColorDim, kRamVramLowThreshold, kRamVramWarnThreshold, kRamWarnColor, forceRedraw);
    drawRow(rightPlotX_, bottomPlotY_, rightCols_, vramLoadHistory_, lastVramCol_, kVramColor,
            kVramColorDim, kRamVramLowThreshold, kRamVramWarnThreshold, kVramWarnColor,
            forceRedraw);
    lcd->endWrite();

    clearDirty();
}
