#include "ThreadsWidget.h"

ThreadsWidget::ThreadsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                             uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                             AppConfigInterface& config, ApplicationMetrics& systemMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics),
      barWidth_(dims.width / config_.getPcMetricsCores()),
      previousBarHeights_(config_.getPcMetricsCores(), 0),
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
    if (pcMetrics_.is_available) {
        updateSmoothedValues();
    }
}

void ThreadsWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    // Clear the widget area once
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);
    isStaticDrawn_ = true;
    clearDirty();
}

void ThreadsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    if (pcMetrics_.is_available) {
        updateSmoothedValues();
    }

    drawBars();
    lastUpdateTimeMs_ = millis();
    clearDirty();

    systemMetrics_.addThreadWidgetFrameTime(1);
}

void ThreadsWidget::updateSmoothedValues() {
    valueSmoother_->update(pcMetrics_.cpu_thread_load, config_.getPcMetricsCores());
    valueSmoother_->getSmoothedValues(smoothedThreadLoads_.data(), config_.getPcMetricsCores());
}

void ThreadsWidget::drawBars() {
    const uint16_t maxBarHeight = dimensions_.height - 1;
    LGFX* lcd = getLcd();

    for (int i = 0; i < config_.getPcMetricsCores(); ++i) {
        uint8_t threadLoad = smoothedThreadLoads_[i];
        uint16_t newHeight = static_cast<uint16_t>(threadLoad * (maxBarHeight - 1) / 100);
        newHeight = min(newHeight, maxBarHeight);
        newHeight = newHeight + 1;  // Ensure minimum visible height

        uint16_t x = dimensions_.x + i * barWidth_;

        lcd->fillRect(x, dimensions_.y, barWidth_ - 1, maxBarHeight, TFT_BLACK);

        // Draw new bar
        if (newHeight > 0) {
            uint16_t color = context_.getColors().getColorFromPercent(threadLoad, false);
            lcd->fillRect(x, dimensions_.y + maxBarHeight - newHeight, barWidth_ - 1, newHeight,
                          color);
        }

        previousBarHeights_[i] = newHeight;
    }
}

bool ThreadsWidget::needsUpdate() const {
    if (!isInitialized_) {
        return false;
    }
    return true;
}

bool ThreadsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}