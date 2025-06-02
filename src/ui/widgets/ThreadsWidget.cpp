#include "ThreadsWidget.h"

ThreadsWidget::ThreadsWidget(const Dimensions& dims, uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics) {}

void ThreadsWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;

    logger_->debug("ThreadsWidget::drawStatic - Drawing static widget");

    // Draw a border around the widget area
    lcd_->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_GREEN);
}

void ThreadsWidget::draw(bool forceRedraw) {
    if (!initialized_ || !lcd_) return;

    bool needsRedraw = forceRedraw || needsUpdate();

    logger_->debug("ThreadsWidget::draw");

    if (needsRedraw && pcMetrics_.is_available) {
        drawBars(forceRedraw);
        lastUpdateTimestamp_ = pcMetrics_.last_update_timestamp;
        lastUpdateTimeMs_ = millis();
    }
}

void ThreadsWidget::drawBars(bool forceRedraw) {
    // Clear the widget area
    lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    // Calculate bar properties
    const uint16_t barWidth = dimensions_.width / 20; // 20 threads
    const uint16_t maxBarHeight = dimensions_.height; // 40 pixels as specified
    const float scale = maxBarHeight / 100.0f; // Scale 0-100% to 0-25 pixels

    for (int i = 0; i < 20; ++i) {
        float load = pcMetrics_.cpu_thread_load[i];
        
        uint16_t barHeight = static_cast<uint16_t>(load * scale);
        if (barHeight > maxBarHeight) barHeight = maxBarHeight; // Clamp to max height

        // Determine color based on load
        uint16_t color;
        if (load < 25.0f) {
            color = TFT_DARKGREEN;
        } else if (load < 50.0f) {
            color = TFT_GREEN;
        } else if (load < 80.0f) {
            color = TFT_YELLOW;
        } else {
            color = TFT_RED;
        }

        // Draw bar
        uint16_t x = dimensions_.x + i * barWidth;
        uint16_t y = dimensions_.y + (maxBarHeight - barHeight); // Draw from bottom up
        lcd_->fillRect(x, y, barWidth - 1, barHeight, color); // -1 for spacing
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
    return false; // No touch interaction
}