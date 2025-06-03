#include "ThreadsWidget.h"

ThreadsWidget::ThreadsWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                             PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics) {}

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
    static uint16_t prevHeights[20] = {0};
    const uint16_t barWidth = dimensions_.width / 20;
    const uint16_t maxBarHeight = dimensions_.height;

    for (int i = 0; i < 20; ++i) {
        uint16_t newHeight = static_cast<uint16_t>(pcMetrics_.cpu_thread_load[i] * maxBarHeight / 100.0f);
        newHeight = min(newHeight, maxBarHeight);
        if (newHeight == 0) newHeight = 1;

        uint16_t x = dimensions_.x + i * barWidth;

        if (forceRedraw || newHeight != prevHeights[i]) {
            // Clear the area between old and new height
            if (newHeight < prevHeights[i]) {
                // Bar shrunk - clear the top part
                lcd_->fillRect(x, dimensions_.y, barWidth - 1, 
                              maxBarHeight - newHeight, TFT_BLACK);
            } else if (newHeight > prevHeights[i]) {
                // Bar grew - clear the area above previous height
                //lcd_->fillRect(x, dimensions_.y, barWidth - 1,
                //              maxBarHeight - prevHeights[i], TFT_BLACK);
            }

            // Draw new bar
            uint16_t color;
            if (newHeight == 1) {
                color = TFT_DARKGREY;
            } else if (pcMetrics_.cpu_thread_load[i] < 25.0f) {
                color = TFT_DARKGREEN;
            } else if (pcMetrics_.cpu_thread_load[i] < 50.0f) {
                color = TFT_GREEN;
            } else if (pcMetrics_.cpu_thread_load[i] < 80.0f) {
                color = TFT_YELLOW;
            } else {
                color = TFT_RED;
            }
            lcd_->fillRect(x, dimensions_.y + maxBarHeight - newHeight,
                         barWidth - 1, newHeight, color);

            prevHeights[i] = newHeight;
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