#include "PcMetricsWidget.h"

PcMetricsWidget::PcMetricsWidget(DisplayContext& context, const Dimensions& dims,
                                 uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                                 AppConfigInterface& config)
    : Widget(dims, updateIntervalMs), context_(context), pcMetrics_(pcMetrics), config_(config) {
    threadsWidget_ = std::make_unique<ThreadsWidget>(
        context_, Dimensions{0, 125 - 65, 480, 55 + 65}, updateIntervalMs, pcMetrics_, config_);

    cpuLoadWidget_ = std::make_unique<SingleValueWidget>(context_, Dimensions{380, 0, 100, 20},
                                                         updateIntervalMs);
    cpuLoadWidget_->setUnit("%");
    cpuLoadWidget_->setRange(0, 100);
    cpuLoadWidget_->setColorThresholds(0.5f, 0.75f, 1.0f);
    cpuLoadWidget_->setLabel("CPU");
    cpuLoadWidget_->setLabelWidth(44);

    gpu3dWidget_ = std::make_unique<SingleValueWidget>(context_, Dimensions{380, 20, 100, 20},
                                                       updateIntervalMs);
    gpu3dWidget_->setUnit("%");
    gpu3dWidget_->setRange(0, 100);
    gpu3dWidget_->setColorThresholds(0.5f, 0.75f, 1.0f);
    gpu3dWidget_->setLabel("3D");
    gpu3dWidget_->setLabelWidth(44);

    gpuComputeWidget_ = std::make_unique<SingleValueWidget>(context_, Dimensions{380, 40, 100, 20},
                                                            updateIntervalMs);
    gpuComputeWidget_->setUnit("%");
    gpuComputeWidget_->setRange(0, 100);
    gpuComputeWidget_->setColorThresholds(0.5f, 0.75f, 1.0f);
    gpuComputeWidget_->setLabel("CMP");
    gpuComputeWidget_->setLabelWidth(44);
}

void PcMetricsWidget::drawStatic() {
    if (!isInitialized_ || !lcd_)
        return;
    // lcd_->drawRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
    //                TFT_RED);

    if (threadsWidget_) {
        threadsWidget_->initialize(context_);
        threadsWidget_->drawStatic();
    }

    if (cpuLoadWidget_) {
        cpuLoadWidget_->initialize(context_);
        cpuLoadWidget_->drawStatic();
    }

    if (gpu3dWidget_) {
        gpu3dWidget_->initialize(context_);
        gpu3dWidget_->drawStatic();
    }

    if (gpuComputeWidget_) {
        gpuComputeWidget_->initialize(context_);
        gpuComputeWidget_->drawStatic();
    }
}

void PcMetricsWidget::draw(bool forceRedraw /* = false */) {
    if (!isInitialized_ || !lcd_)
        return;

    bool needsRedraw = forceRedraw || needsUpdate();
    if (needsRedraw && pcMetrics_.is_available) {
        char buffer[32];  // Reusable buffer for strings

        // Update CPU load widget
        if (cpuLoadWidget_) {
            cpuLoadWidget_->setValue(pcMetrics_.cpu_load);
            cpuLoadWidget_->draw(forceRedraw);
        }

        // Update GPU 3D widget
        if (gpu3dWidget_) {
            gpu3dWidget_->setValue(pcMetrics_.gpu_3d);
            gpu3dWidget_->draw(forceRedraw);
        }

        // Update GPU Compute widget
        if (gpuComputeWidget_) {
            gpuComputeWidget_->setValue(pcMetrics_.gpu_compute);
            gpuComputeWidget_->draw(forceRedraw);
        }

        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextSize(2);
        lcd_->setTextDatum(TL_DATUM);

        // Draw GPU mem - OPTIMIZED
        snprintf(buffer, sizeof(buffer), "GPU RAM: %d%%  ", pcMetrics_.gpu_mem);
        lcd_->drawString(buffer, dimensions_.x + 2, dimensions_.y + 0 + 2);

        // Draw RAM Load - OPTIMIZED
        snprintf(buffer, sizeof(buffer), "RAM: %d%%  ", pcMetrics_.mem_load);
        lcd_->drawString(buffer, dimensions_.x + 2, dimensions_.y + 25 + 2);

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
    if (!isInitialized_ || !pcMetrics_.is_available ||
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