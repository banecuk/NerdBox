#include "PcMetricsWidget.h"

PcMetricsWidget::PcMetricsWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                                 PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs), pcMetrics_(pcMetrics) {
    threadsWidget_ = std::make_unique<ThreadsWidget>(Dimensions{0, 125, 480, 55},
                                                     updateIntervalMs, pcMetrics_);

    cpuLoadWidget_ = std::make_unique<SingleValueWidget>(Dimensions{400, 0, 80, 20}, updateIntervalMs);
    cpuLoadWidget_->setUnit("%");
    cpuLoadWidget_->setRange(0, 100);
    cpuLoadWidget_->setColorThresholds(0.5f, 0.75f, 1.0f);                                                     

    gpu3dWidget_ = std::make_unique<SingleValueWidget>(Dimensions{400, 20, 80, 20}, updateIntervalMs);
    gpu3dWidget_->setUnit("%");
    gpu3dWidget_->setRange(0, 100);
    gpu3dWidget_->setColorThresholds(0.5f, 0.75f, 1.0f);  
}

void PcMetricsWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;
    // lcd_->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
    //                TFT_RED);

    if (threadsWidget_) {
        threadsWidget_->initialize(lcd_, *logger_);
        threadsWidget_->drawStatic();
    }

    if (cpuLoadWidget_) {
        cpuLoadWidget_->initialize(lcd_, *logger_);
        cpuLoadWidget_->drawStatic();
    }

    if (gpu3dWidget_) {
        gpu3dWidget_->initialize(lcd_, *logger_);
        gpu3dWidget_->drawStatic();
    }
}

void PcMetricsWidget::draw(bool forceRedraw /* = false */) {
    if (!initialized_ || !lcd_) return;

    bool needsRedraw = forceRedraw || needsUpdate();

    if (needsRedraw && pcMetrics_.is_available) {  // TODO clear the area if not available
        // lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width,
        //                dimensions_.height, TFT_BLACK);

        // Update CPU load widget
        if (cpuLoadWidget_) {
            cpuLoadWidget_->setValue(pcMetrics_.cpu_load);
            cpuLoadWidget_->draw(forceRedraw);
        }

        // Update GPU load widget
        if (gpu3dWidget_) {
            gpu3dWidget_->setValue(pcMetrics_.gpu_3d);
            gpu3dWidget_->draw(forceRedraw);
        }



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

        // Draw ThreadsWidget
        if (threadsWidget_) {
            threadsWidget_->draw(forceRedraw);
        }

        // last update of the data
        lastUpdateTimestamp_ = pcMetrics_.last_update_timestamp;

        // last update of the widget
        lastUpdateTimeMs_ = millis();
    }
}

bool PcMetricsWidget::needsUpdate() const {
    if (!initialized_ || !pcMetrics_.is_available ||
        !(pcMetrics_.last_update_timestamp > lastUpdateTimestamp_)) {
        return false;
    }
    return true;
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    if (threadsWidget_ && threadsWidget_->handleTouch(x, y)) {
        return true;
    }
    return false;  // No touch interaction
}