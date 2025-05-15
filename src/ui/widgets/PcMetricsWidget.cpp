#include "PcMetricsWidget.h"

PcMetricsWidget::PcMetricsWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                                 PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics) {}


void PcMetricsWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;

    lcd_->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_RED);
    // lcd_->fillRect(dimensions_.x + 1, dimensions_.y + 1, dimensions_.width - 2, dimensions_.height - 2, TFT_BLACK);
}

void PcMetricsWidget::draw(bool forceRedraw /* = false */) {
    if (!initialized_ || !lcd_) return;

    bool needsRedraw = forceRedraw || needsUpdate();

    if (needsRedraw && pcMetrics_.is_available) {
        // lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width,
        //                dimensions_.height, TFT_BLACK);

        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextSize(2);
        lcd_->setTextDatum(TL_DATUM);

        // Draw CPU Load
        String cpuLabel = "CPU: " + String(pcMetrics_.cpu_load) + "%  ";
        lcd_->drawString(cpuLabel.c_str(), dimensions_.x + 2, dimensions_.y + 0 + 2);

        // Draw GPU Load
        String gpu3D = "GPU 3D: " + String(pcMetrics_.gpu_3d) + "%  ";
        lcd_->drawString(gpu3D.c_str(), dimensions_.x + 2, dimensions_.y + 25 + 2);

        // Draw GPU Load
        String gpuCompute = "GPU Compute: " + String(pcMetrics_.gpu_compute) + "%  ";
        lcd_->drawString(gpuCompute.c_str(), dimensions_.x + 2, dimensions_.y + 50 + 2);

        // Draw GPU mem
        String gpuMem = "GPU RAM: " + String(pcMetrics_.gpu_mem) + "%  ";
        lcd_->drawString(gpuMem.c_str(), dimensions_.x + 2, dimensions_.y + 75 + 2);        

        // Draw RAM Load
        String ram = "RAM: " + String(pcMetrics_.mem_load) + "%  ";
        lcd_->drawString(ram.c_str(), dimensions_.x + 2, dimensions_.y + 100 + 2);

        pcMetrics_.is_updated = false;  // Reset update flag
        lastUpdateTimeMs_ = millis();
    }
}

bool PcMetricsWidget::needsUpdate() const {
    if (!initialized_ || !pcMetrics_.is_available || !pcMetrics_.is_updated) {
        return false;
    }
    return true;
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;  // No touch interaction
}