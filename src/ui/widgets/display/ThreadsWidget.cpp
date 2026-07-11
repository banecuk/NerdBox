#include "ThreadsWidget.h"

#include <algorithm>

ThreadsWidget::ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                             uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                             AppConfigInterface& config, ApplicationMetrics& systemMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics),
      freshnessGuard_(pcMetrics.is_available, pcMetrics.last_update_timestamp),
      barWidth_(dims.width / config_.getPcMetricsCores()),
      previousBarHeights_(config_.getPcMetricsCores(), 0),
      previousColors_(config_.getPcMetricsCores(), 0),
      smoothedThreadLoads_(config_.getPcMetricsCores(), 0) {
    // Initialize value smoother with configurable parameters
    valueSmoother_ = std::make_unique<ValueSmoother>(
        config_.getPcMetricsCores(), config_.getHardwareMonitorThreadsUpwardDecay(),
        config_.getHardwareMonitorThreadsDownwardDecay());
}

void ThreadsWidget::initialize(DisplayContext& context) {
    Widget::initialize(context);

    // Set update interval to the faster threads refresh rate for high FPS animation
    setUpdateInterval(config_.getHardwareMonitorThreadsRefreshMs());

    // Initialize the value smoother with current data if available
    if (freshnessGuard_.isFresh()) {
        updateSmoothedValues();
    }
}

void ThreadsWidget::onDrawStatic() {
    // Clear the widget area once
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Reset cached state so the next draw treats every bar as changed
    std::fill(previousBarHeights_.begin(), previousBarHeights_.end(), 0);
    std::fill(previousColors_.begin(), previousColors_.end(), 0);
}

void ThreadsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    if (freshnessGuard_.isFresh()) {
        updateSmoothedValues();
    }

    drawBars();
    lastUpdateTimeMs_ = millis();
    clearDirty();

    systemMetrics_.addThreadWidgetFrameTime();
}

void ThreadsWidget::updateSmoothedValues() {
    // Called every screen tick (kThreadsRefreshMs), not just when a new fetch
    // lands (kRefreshMs). This is intentional: the raw target stays constant
    // between fetches, so repeated calls just keep nudging the smoothed value
    // toward it, and the kThreadsUpward/DownwardSmoothing alphas are tuned for
    // this per-tick cadence to produce a fast-attack / slow-decay VU-meter
    // animation. Gating this on pcMetrics_.last_update_timestamp would turn
    // that smooth animation into a hard step every fetch instead.
    valueSmoother_->update(pcMetrics_.cpu_thread_load, config_.getPcMetricsCores());
    valueSmoother_->getSmoothedValues(smoothedThreadLoads_.data(), config_.getPcMetricsCores());
}

void ThreadsWidget::drawBars() {
    const uint16_t maxBarHeight = dimensions_.height - 1;
    LGFX* lcd = getLcd();
    const int coreCount = config_.getPcMetricsCores();

    for (int i = 0; i < coreCount; ++i) {
        const uint8_t threadLoad = smoothedThreadLoads_[i];
        uint16_t newHeight = static_cast<uint16_t>(threadLoad * (maxBarHeight - 1) / 100);
        newHeight = min(newHeight, maxBarHeight);
        newHeight = newHeight + 1;  // Ensure minimum visible height

        const uint16_t newColor = context_.getColors().getColorFromPercent(threadLoad, false);
        const uint16_t oldHeight = previousBarHeights_[i];
        const uint16_t oldColor = previousColors_[i];

        const uint16_t x = dimensions_.x + i * barWidth_;
        const uint16_t w = barWidth_ - 1;

        if (newHeight == oldHeight && newColor == oldColor) {
            continue;  // Bar unchanged — no pixel writes needed
        }

        if (newColor != oldColor) {
            // Color threshold crossed: repaint the entire bar in the new color,
            // then erase any excess from the old (taller) portion.
            if (newHeight > 0) {
                lcd->fillRect(x, dimensions_.y + maxBarHeight - newHeight, w, newHeight, newColor);
            }
            if (newHeight < oldHeight) {
                // Erase the strip above the new top
                lcd->fillRect(x, dimensions_.y + maxBarHeight - oldHeight, w, oldHeight - newHeight,
                              TFT_BLACK);
            }
        } else if (newHeight > oldHeight) {
            // Bar grew — fill only the new top strip, no erase needed
            const uint16_t delta = newHeight - oldHeight;
            lcd->fillRect(x, dimensions_.y + maxBarHeight - newHeight, w, delta, newColor);
        } else {
            // Bar shrank — erase only the vacated top strip
            const uint16_t delta = oldHeight - newHeight;
            lcd->fillRect(x, dimensions_.y + maxBarHeight - oldHeight, w, delta, TFT_BLACK);
        }

        previousBarHeights_[i] = newHeight;
        previousColors_[i] = newColor;
    }
}

bool ThreadsWidget::needsUpdate() const {
    if (!isInitialized_) {
        return false;
    }
    return Widget::needsUpdate();
}

bool ThreadsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}