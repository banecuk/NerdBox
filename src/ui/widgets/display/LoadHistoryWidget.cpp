#include "LoadHistoryWidget.h"

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

static constexpr uint16_t kBgColor = TFT_BLACK;
static constexpr uint16_t kCpuColor = 0xC618;
static constexpr uint16_t kGpuColor = 0xB471;

LoadHistoryWidget::LoadHistoryWidget(DisplayContext& context,
                                     const WidgetInterface::Dimensions& dims,
                                     uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.is_available, pcMetrics.last_update_timestamp) {}

void LoadHistoryWidget::computePlotLayout() {
    plotX_ = dimensions_.x + kCaptionWidth;
    plotY_ = dimensions_.y + 2;
    plotHeight_ = dimensions_.height - 4;
    plotWidth_ = kHistorySize * kColWidth;
    // kHistorySize is sized so this is normally a no-op — clamp only guards
    // against a narrower-than-designed dims override.
    const uint16_t maxWidth = dimensions_.width - kCaptionWidth - kRightMargin;
    if (plotWidth_ > maxWidth)
        plotWidth_ = maxWidth;
}

void LoadHistoryWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    computePlotLayout();
    drawCaptionLabel("LOAD");
    lcd->drawRect(plotX_ - 1, plotY_ - 1, plotWidth_ + 2, plotHeight_ + 2, Colors::kHairline);

    for (auto& h : lastCpuHeight_) h = 0;
    for (auto& h : lastGpuHeight_) h = 0;
}

void LoadHistoryWidget::sampleIfNeeded() {
    if (!freshnessGuard_.isFresh())
        return;
    if (pcMetrics_.last_update_timestamp == lastSampledTimestamp_)
        return;

    lastSampledTimestamp_ = pcMetrics_.last_update_timestamp;
    cpuHistory_.push(pcMetrics_.cpu_load);
    gpuHistory_.push(pcMetrics_.gpu_load);
}

void LoadHistoryWidget::drawPlot(bool forceFullRepaint) {
    LGFX* lcd = getLcd();
    const size_t count = cpuHistory_.size();
    const size_t offset = kHistorySize - count;

    lcd->startWrite();
    for (size_t col = 0; col < kHistorySize; ++col) {
        uint8_t cpuHeight = 0;
        uint8_t gpuHeight = 0;
        if (col >= offset) {
            const size_t idx = col - offset;
            const uint32_t cpuH =
                (static_cast<uint32_t>(cpuHistory_.at(idx)) * (plotHeight_ - 1)) / 100;
            const uint32_t gpuH =
                (static_cast<uint32_t>(gpuHistory_.at(idx)) * (plotHeight_ - 1)) / 100;
            cpuHeight = static_cast<uint8_t>((cpuH > plotHeight_ - 1 ? plotHeight_ - 1 : cpuH) + 1);
            gpuHeight = static_cast<uint8_t>((gpuH > plotHeight_ - 1 ? plotHeight_ - 1 : gpuH) + 1);
        }

        const uint8_t oldCpu = lastCpuHeight_[col];
        const uint8_t oldGpu = lastGpuHeight_[col];
        if (!forceFullRepaint && cpuHeight == oldCpu && gpuHeight == oldGpu)
            continue;

        const uint16_t x = plotX_ + static_cast<uint16_t>(col) * kColWidth;
        const uint16_t w = kColWidth - 1;

        // Redraw the whole column: clear, then CPU fill bar, then GPU marker line.
        lcd->fillRect(x, plotY_, w, plotHeight_, kBgColor);
        if (cpuHeight > 0)
            lcd->fillRect(x, plotY_ + plotHeight_ - cpuHeight, w, cpuHeight, kCpuColor);
        if (gpuHeight > 0) {
            const uint16_t gpuY = plotY_ + plotHeight_ - gpuHeight;
            lcd->fillRect(x, gpuY, w, 1, kGpuColor);
        }

        lastCpuHeight_[col] = cpuHeight;
        lastGpuHeight_[col] = gpuHeight;
    }
    lcd->endWrite();
}

void LoadHistoryWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    sampleIfNeeded();
    drawPlot(forceRedraw);

    lastUpdateTimeMs_ = millis();
    clearDirty();
}

bool LoadHistoryWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
