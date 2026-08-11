#include "ThreadsWidget.h"

#include <algorithm>

#include "core/resources/FontRegistry.h"
#include "ui/core/UiText.h"

ThreadsWidget::ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                             uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                             const AppSettings& config, ApplicationMetrics& systemMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics),
      freshnessGuard_(pcMetrics.freshness),
      barWidth_(dims.width / config_.pcMetricsCores),
      previousBarHeights_(config_.pcMetricsCores, 0),
      previousColors_(config_.pcMetricsCores, 0),
      smoothedThreadLoads_(config_.pcMetricsCores, 0) {
    // Initialize value smoother with configurable parameters
    valueSmoother_ = std::make_unique<ValueSmoother>(
        config_.pcMetricsCores, config_.hardwareMonitorThreadsUpwardSmoothing,
        config_.hardwareMonitorThreadsDownwardSmoothing);
}

void ThreadsWidget::initialize(DisplayContext& context) {
    Widget::initialize(context);

    // Set update interval to the faster threads refresh rate for high FPS animation
    setUpdateInterval(config_.hardwareMonitorThreadsRefreshMs);

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

    const bool fresh = freshnessGuard_.isFresh();

    if (!wasFresh_ && fresh) {
        // Coming back from stale: reset cached bar state so the next
        // drawBars() treats every bar as changed instead of diffing against
        // pre-outage heights/colors left over from before the "No Data"
        // message was shown.
        onDrawStatic();
    }

    if (fresh) {
        updateSmoothedValues();
        systemMetrics_.addThreadWidgetFrameTime();
        drawBars();
    } else if (wasFresh_) {
        // Just went stale: replace the frozen bars with an explicit message
        // instead of leaving them looking like live (if dim) data.
        drawNoDataMessage();
    }

    wasFresh_ = fresh;
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void ThreadsWidget::updateSmoothedValues() {
    // Called every screen tick (kThreadsRefreshMs), not just when a new fetch
    // lands (kRefreshMs). This is intentional: the raw target stays constant
    // between fetches, so repeated calls just keep nudging the smoothed value
    // toward it, and the kThreadsUpward/DownwardSmoothing alphas are tuned for
    // this per-tick cadence to produce a fast-attack / slow-decay VU-meter
    // animation. Gating this on pcMetrics_.freshness.lastUpdateMs() would turn
    // that smooth animation into a hard step every fetch instead.
    valueSmoother_->update(pcMetrics_.cpu_thread_load, config_.pcMetricsCores);
    valueSmoother_->getSmoothedValues(smoothedThreadLoads_.data(), config_.pcMetricsCores);
}

void ThreadsWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadMetric(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

void ThreadsWidget::drawBars() {
    const uint16_t maxBarHeight = dimensions_.height - 1;
    LGFX* lcd = getLcd();
    const int coreCount = config_.pcMetricsCores;

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

    const bool fresh = freshnessGuard_.isFresh();
    if (fresh != wasFresh_) {
        return true;  // one redraw to switch between bars and "No Data"
    }
    if (!fresh) {
        return false;  // "No Data" already shown; nothing changes while stale
    }
    return Widget::needsUpdate();
}

bool ThreadsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}