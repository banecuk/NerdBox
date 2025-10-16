#include "ThreadsWidget.h"

ThreadsWidget::ThreadsWidget(DisplayContext& context, const Dimensions& dims,
                             uint32_t updateIntervalMs, PcMetrics& pcMetrics, AppConfigInterface& config)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      barWidth_(dims.width / Config::PcMetrics::kCores),
      config_(config) {}

void ThreadsWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;
}

void ThreadsWidget::draw(bool forceRedraw) {
    if (!initialized_ || !lcd_) return;

    bool needsRedraw = forceRedraw || needsUpdate();

    if (needsRedraw && pcMetrics_.is_available) {
        drawBars(forceRedraw);
        lastUpdateTimestamp_ = pcMetrics_.last_update_timestamp;
        lastUpdateTimeMs_ = millis();
    }
}

void ThreadsWidget::drawBars(bool forceRedraw) {
    const uint16_t maxBarHeight = dimensions_.height;

    for (int i = 0; i < config_.getPcMetricsCores(); ++i) {
        uint16_t newHeight =
            static_cast<uint16_t>(pcMetrics_.cpu_thread_load[i] * maxBarHeight / 100);
        ;
        newHeight = min(newHeight, maxBarHeight);
        if (newHeight == 0) newHeight = 1;

        uint16_t x = dimensions_.x + i * barWidth_;

        if (forceRedraw || newHeight != previousBarHeights_[i]) {
            // Clear the area between old and new height when bar shrinks
            if (newHeight < previousBarHeights_[i]) {
                lcd_->fillRect(x, dimensions_.y, barWidth_ - 1, maxBarHeight - newHeight,
                               TFT_BLACK);
            }

            // Draw new bar
            uint16_t color;
            if (newHeight == 1) {
                color = TFT_DARKGREY;
            } else {
                color = context_.getColors().getColorFromPercent(
                    pcMetrics_.cpu_thread_load[i], false);
            }

            lcd_->fillRect(x, dimensions_.y + maxBarHeight - newHeight, barWidth_ - 1,
                           newHeight, color);

            previousBarHeights_[i] = newHeight;
        }
    }
}
bool ThreadsWidget::needsUpdate() const {
    if (!initialized_ || !pcMetrics_.is_available ||
        !(pcMetrics_.last_update_timestamp > lastUpdateTimestamp_)) {
        return false;
    }
    return true;
}

bool ThreadsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;  // No touch interaction
}