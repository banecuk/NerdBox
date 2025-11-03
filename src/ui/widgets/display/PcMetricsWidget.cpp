#include "PcMetricsWidget.h"

#include <cstdio>

PcMetricsWidget::PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                                 uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                                 AppConfigInterface& config, ApplicationMetrics& systemMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics) {
    const u8_t metricWidth = 86;
    const u8_t metricLabelWidth = 26;

    const u16_t guidelineY1 = 0;
    const u16_t guidelineY2 = 30;
    const u16_t guidelineY3 = 60;
    const u16_t guidelineY4 = 90;
    const u16_t guidelineY5 = 120;

    const u16_t guidelineX4 = 0;
    const u16_t guidelineX5 = 480 - metricWidth * 5;
    const u16_t guidelineX6 = 480 - metricWidth * 4;
    const u16_t guidelineX7 = 480 - metricWidth * 3;
    const u16_t guidelineX8 = 480 - metricWidth * 2;
    const u16_t guidelineX9 = 480 - metricWidth;

    // CPU widgets using builder pattern
    cpuLoadWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX8, guidelineY1, metricWidth,
                                                          guidelineY2 - guidelineY1},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("CPU")
            .labelWidth(metricLabelWidth)
            .textSize(2)
            .build();

    cpuTemperatureWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX9, guidelineY1, metricWidth,
                                                          guidelineY2 - guidelineY1},
                              updateIntervalMs)
            .unit(" C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(metricLabelWidth)
            .build();

    cpuPowerWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX8, guidelineY2, metricWidth,
                                                          guidelineY3 - guidelineY2},
                              updateIntervalMs)
            .unit(" W")
            .range(0, 400)
            .colorThresholds(55.0f, 140.0f)
            .label("PWR")
            .labelWidth(metricLabelWidth)
            .build();

    cpuFanWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX9, guidelineY2, metricWidth,
                                                          guidelineY3 - guidelineY2},
                              updateIntervalMs)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1200.0f)
            .label("FAN")
            .labelWidth(metricLabelWidth)
            .build();

    // GPU widgets using builder pattern
    gpuLoadWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX8, guidelineY3, metricWidth,
                                                          guidelineY4 - guidelineY3},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("GPU")
            .labelWidth(metricLabelWidth)
            .textSize(2)
            .build();

    gpuTemperatureWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX9, guidelineY3, metricWidth,
                                                          guidelineY4 - guidelineY3},
                              updateIntervalMs)
            .unit(" C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(metricLabelWidth)
            .build();

    gpuPowerWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX8, guidelineY4, metricWidth,
                                                          guidelineY5 - guidelineY4},
                              updateIntervalMs)
            .unit(" W")
            .range(0, 400)
            .colorThresholds(50.0f, 170.0f)
            .label("PWR")
            .labelWidth(metricLabelWidth)
            .build();

    gpu3dWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX7, guidelineY3, metricWidth,
                                                          guidelineY4 - guidelineY3},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("3D")
            .labelWidth(metricLabelWidth)
            .build();

    gpuComputeWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX7, guidelineY4, metricWidth,
                                                          guidelineY5 - guidelineY4},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("CMP")
            .labelWidth(metricLabelWidth)
            .build();

    gpuMemoryWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX6, guidelineY3, metricWidth,
                                                          guidelineY5 - guidelineY3},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(30.0f, 90.0f)
            .label("MEM")
            .labelWidth(metricLabelWidth)
            .textSize(2)
            .build();

    gpuFanWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX9, guidelineY4, metricWidth,
                                                          guidelineY5 - guidelineY4},
                              updateIntervalMs)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1200.0f)
            .label("FAN")
            .labelWidth(metricLabelWidth)
            .build();

    // Memory widget using builder pattern
    memoryLoadWidget_ =
        MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX5, guidelineY3, metricWidth,
                                                          guidelineY5 - guidelineY3},
                              updateIntervalMs)
            .unit("%")
            .range(0, 100)
            .colorThresholds(60.0f, 90.0f)
            .label("RAM")
            .labelWidth(metricLabelWidth)
            .textSize(2)
            .build();

    // System fan widgets using builder pattern
    fanWidget1_ = MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX4, guidelineY3,
                                                                    guidelineX5 - guidelineX4,
                                                                    guidelineY4 - guidelineY3},
                                        updateIntervalMs)
                      .unit("")
                      .range(0, 1200)
                      .colorThresholds(720.0f, 1000.0f)
                      .label("F1")
                      .labelWidth(14)
                      .build();

    fanWidget2_ = MetricWidget::Builder(WidgetInterface::Dimensions{guidelineX4, guidelineY4,
                                                                    guidelineX5 - guidelineX4,
                                                                    guidelineY5 - guidelineY4},
                                        updateIntervalMs)
                      .unit("")
                      .range(0, 1200)
                      .colorThresholds(820.0f, 1000.0f)
                      .label("F2")
                      .labelWidth(14)
                      .build();
}

void PcMetricsWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    // Only draw static elements if we have fresh data available
    if (hasFreshData()) {
        // Helper lambda
        auto initAndDrawStatic = [this](const std::unique_ptr<MetricWidget>& widget) {
            if (widget) {
                widget->initialize(context_);
                widget->drawStatic();
            }
        };

        // CPU widgets
        initAndDrawStatic(cpuLoadWidget_);
        initAndDrawStatic(cpuTemperatureWidget_);
        initAndDrawStatic(cpuPowerWidget_);
        initAndDrawStatic(cpuFanWidget_);

        // GPU widgets
        initAndDrawStatic(gpuLoadWidget_);
        initAndDrawStatic(gpuPowerWidget_);
        initAndDrawStatic(gpuTemperatureWidget_);
        initAndDrawStatic(gpu3dWidget_);
        initAndDrawStatic(gpuComputeWidget_);
        initAndDrawStatic(gpuFanWidget_);
        initAndDrawStatic(gpuMemoryWidget_);

        // Memory widget
        initAndDrawStatic(memoryLoadWidget_);

        // System fan widgets
        initAndDrawStatic(fanWidget1_);
        initAndDrawStatic(fanWidget2_);

        isStaticDrawn_ = true;
        clearDirty();
        if (getLogger()) {
            getLogger()->debug("PcMetricsWidget: Static elements drawn - data available");
        }
    } else {
        // Clear the display and show "No Data" message
        clearAllWidgets();
        drawNoDataMessage();

        if (getLogger()) {
            getLogger()->debug(
                "PcMetricsWidget: No fresh data available - showing 'No Data' message");
        }
    }
}

void PcMetricsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    bool currentlyHasFreshData = hasFreshData();
    bool stateChanged = (wasFreshData_ != currentlyHasFreshData);

    // Handle state transitions
    if (stateChanged) {
        if (!currentlyHasFreshData) {
            clearAllWidgets();
            drawNoDataMessage();
            if (getLogger()) {
                getLogger()->debug(
                    "PcMetricsWidget: No fresh data available - showing 'No Data' message");
            }
        } else {
            restoreStaticDisplay();
            if (getLogger()) {
                getLogger()->debug(
                    "PcMetricsWidget: Fresh data available - drawing static elements");
            }
        }
        wasFreshData_ = currentlyHasFreshData;
    }

    // Only update dynamic content if we have fresh data and need to redraw
    bool needsRedraw = forceRedraw || isDirty() || needsUpdate();
    if (currentlyHasFreshData && needsRedraw) {
        drawDynamicData();
        clearDirty();
    }

    lastUpdateTimeMs_ = millis();
}

void PcMetricsWidget::drawDynamicData() {
    // Only draw dynamic data if static elements are already drawn
    if (!isStaticDrawn_) {
        return;
    }

    // Helper lambda
    auto updateAndDraw = [](const std::unique_ptr<MetricWidget>& widget, float value) {
        if (widget) {
            widget->setValue(value);
            widget->draw(false);
        }
    };

    // CPU widgets
    updateAndDraw(cpuLoadWidget_, pcMetrics_.cpu_load);
    updateAndDraw(cpuTemperatureWidget_, pcMetrics_.cpu_temperature);
    updateAndDraw(cpuPowerWidget_, pcMetrics_.cpu_power);
    updateAndDraw(cpuFanWidget_, pcMetrics_.cpu_fan);

    // GPU widgets
    updateAndDraw(gpuLoadWidget_, pcMetrics_.gpu_load);
    updateAndDraw(gpuTemperatureWidget_, pcMetrics_.gpu_temperature);
    updateAndDraw(gpuPowerWidget_, pcMetrics_.gpu_power);
    updateAndDraw(gpu3dWidget_, pcMetrics_.gpu_3d);
    updateAndDraw(gpuComputeWidget_, pcMetrics_.gpu_compute);
    updateAndDraw(gpuMemoryWidget_, pcMetrics_.gpu_mem);
    updateAndDraw(gpuFanWidget_, pcMetrics_.gpu_fan);

    // Memory widget
    updateAndDraw(memoryLoadWidget_, pcMetrics_.mem_load);

    // System fan widgets
    if (fanWidget1_) {
        fanWidget1_->setValue(pcMetrics_.system_fans[0]);
        fanWidget1_->draw(false);
    }
    if (fanWidget2_) {
        fanWidget2_->setValue(pcMetrics_.system_fans[4]);
        fanWidget2_->draw(false);
    }

    lastUpdateTimestamp_ = pcMetrics_.last_update_timestamp;
}

void PcMetricsWidget::drawNoDataMessage() {
    getLcd()->setTextColor(TFT_DARKGREY, TFT_BLACK);
    getLcd()->setTextSize(2);
    getLcd()->setTextDatum(MC_DATUM);

    getLcd()->drawString("No Data", dimensions_.x + dimensions_.width / 2,
                         dimensions_.y + dimensions_.height / 2);
}

void PcMetricsWidget::clearAllWidgets() {
    // Clear the entire widget area
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    // Clear all child widget areas
    auto clearWidget = [this](const std::unique_ptr<MetricWidget>& widget) {
        if (widget) {
            auto dims = widget->getDimensions();
            getLcd()->fillRect(dims.x, dims.y, dims.width, dims.height, TFT_BLACK);
        }
    };

    clearWidget(cpuLoadWidget_);
    clearWidget(cpuTemperatureWidget_);
    clearWidget(cpuPowerWidget_);
    clearWidget(cpuFanWidget_);
    clearWidget(gpuLoadWidget_);
    clearWidget(gpuTemperatureWidget_);
    clearWidget(gpuPowerWidget_);
    clearWidget(gpu3dWidget_);
    clearWidget(gpuComputeWidget_);
    clearWidget(gpuMemoryWidget_);
    clearWidget(gpuFanWidget_);
    clearWidget(memoryLoadWidget_);
    clearWidget(fanWidget1_);
    clearWidget(fanWidget2_);

    isStaticDrawn_ = false;
}

void PcMetricsWidget::restoreStaticDisplay() {
    // Clear everything and redraw static elements
    clearAllWidgets();

    // Draw static elements (this will only draw if fresh data is available)
    drawStatic();
}

bool PcMetricsWidget::hasFreshData() const {
    // Data is fresh if it's available and not stale
    return pcMetrics_.is_available && !isDataStale();
}

bool PcMetricsWidget::isDataStale() const {
    // If data is not available at all, it's definitely stale
    if (!pcMetrics_.is_available) {
        return true;
    }

    unsigned long currentTime = millis();
    unsigned long timeSinceLastUpdate = currentTime - pcMetrics_.last_update_timestamp;

    bool stale = (timeSinceLastUpdate > staleTimeoutMs_);

    return stale;
}

bool PcMetricsWidget::needsUpdate() const {
    if (!isInitialized_) {
        return false;
    }

    // Check if state changed
    bool currentlyHasFreshData = hasFreshData();
    bool stateChanged = (wasFreshData_ != currentlyHasFreshData);

    if (stateChanged) {
        return true;
    }

    // Only update dynamic content if we have fresh data and static elements are drawn
    if (currentlyHasFreshData && isStaticDrawn_) {
        return (pcMetrics_.last_update_timestamp > lastUpdateTimestamp_) ||
               (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
    }

    return false;
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}

void PcMetricsWidget::setStaleTimeout(unsigned long timeoutMs) {
    staleTimeoutMs_ = timeoutMs;
    markDirty();
    if (getLogger()) {
        getLogger()->debugf("PcMetricsWidget: Stale timeout set to %lu ms", timeoutMs);
    }
}

unsigned long PcMetricsWidget::getStaleTimeout() const {
    return staleTimeoutMs_;
}

void PcMetricsWidget::showStaleIndicator() {
    if (getLcd()) {
        getLcd()->setTextColor(TFT_ORANGE, TFT_BLACK);
        getLcd()->setTextSize(1);
        getLcd()->setTextDatum(TL_DATUM);
        getLcd()->drawString("STALE", dimensions_.x + 10, dimensions_.y + 10);
    }
}