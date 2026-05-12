#include "PcMetricsWidget.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Constructor — delegates layout work to per-subsystem helpers
// ---------------------------------------------------------------------------

PcMetricsWidget::PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                                 uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                                 AppConfigInterface& config, ApplicationMetrics& systemMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      config_(config),
      systemMetrics_(systemMetrics) {
    buildCpuWidgets();
    buildGpuWidgets();
    buildMemoryWidget();
    buildFanWidgets();
}

// ---------------------------------------------------------------------------
// Widget construction helpers
// ---------------------------------------------------------------------------

void PcMetricsWidget::buildCpuWidgets() {
    // Row 1: CPU load | CPU temperature
    cpuLoadWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol8, kRow1, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("CPU")
            .labelWidth(kLabelWidth)
            .textSize(2)
            .build();

    cpuTemperatureWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow1, kTileWidth, kRowH}, updateIntervalMs_)
            .unit(" C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(kLabelWidth)
            .build();

    // Row 2: CPU power | CPU fan
    cpuPowerWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol8, kRow2, kTileWidth, kRowH}, updateIntervalMs_)
            .unit(" W")
            .range(0, 400)
            .colorThresholds(55.0f, 140.0f)
            .label("PWR")
            .labelWidth(kLabelWidth)
            .build();

    cpuFanWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow2, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1200.0f)
            .label("FAN")
            .labelWidth(kLabelWidth)
            .build();
}

void PcMetricsWidget::buildGpuWidgets() {
    // Row 3: RAM | GPU memory | GPU 3D | GPU load | GPU temperature
    gpuLoadWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol8, kRow3, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("GPU")
            .labelWidth(kLabelWidth)
            .textSize(2)
            .build();

    gpuTemperatureWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow3, kTileWidth, kRowH}, updateIntervalMs_)
            .unit(" C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(kLabelWidth)
            .build();

    // Row 3 (middle): GPU 3D workload
    gpu3dWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol7, kRow3, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("3D")
            .labelWidth(kLabelWidth)
            .build();

    // Row 3–4 span: GPU memory (double height)
    gpuMemoryWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol6, kRow3, kTileWidth, kRowH * 2}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(30.0f, 90.0f)
            .label("MEM")
            .labelWidth(kLabelWidth)
            .textSize(2)
            .build();

    // Row 4: GPU power | GPU fan | GPU compute
    gpuPowerWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol8, kRow4, kTileWidth, kRowH}, updateIntervalMs_)
            .unit(" W")
            .range(0, 400)
            .colorThresholds(50.0f, 170.0f)
            .label("PWR")
            .labelWidth(kLabelWidth)
            .build();

    gpuFanWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow4, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1400.0f)
            .label("FAN")
            .labelWidth(kLabelWidth)
            .build();

    gpuComputeWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol7, kRow4, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("CMP")
            .labelWidth(kLabelWidth)
            .build();
}

void PcMetricsWidget::buildMemoryWidget() {
    // Rows 3–4 span (double height), leftmost of the right-side tiles
    memoryLoadWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol5, kRow3, kTileWidth, kRowH * 2}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(60.0f, 90.0f)
            .label("RAM")
            .labelWidth(kLabelWidth)
            .textSize(2)
            .build();
}

void PcMetricsWidget::buildFanWidgets() {
    // System fans occupy the left column (kColFan), rows 3 and 4.
    // Width spans from kColFan to kCol5 (the start of the metric tiles).
    const uint16_t fanWidth = kCol5 - kColFan;

    fanWidget1_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kColFan, kRow3, fanWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1200)
            .colorThresholds(750.0f, 1000.0f)
            .label("F1")
            .labelWidth(kFanLabelWidth)
            .build();

    fanWidget2_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kColFan, kRow4, fanWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1200)
            .colorThresholds(880.0f, 1200.0f)
            .label("F2")
            .labelWidth(kFanLabelWidth)
            .build();
}

void PcMetricsWidget::ensureDiskWidgetsCreated() {
    // Snapshot everything we need under one lock, then do all widget work lock-free.
    struct DriveSnapshot {
        char name[4];
        int freeSpacePercent;
    };
    std::vector<DriveSnapshot> snapshot;
    {
        PcMetricsDiskLock lock(pcMetrics_);
        const size_t driveCount =
            pcMetrics_.diskDrives.size() < kMaxDiskWidgets ? pcMetrics_.diskDrives.size()
                                                           : kMaxDiskWidgets;
        snapshot.reserve(driveCount);
        for (size_t i = 0; i < driveCount; ++i) {
            DriveSnapshot s;
            strncpy(s.name, pcMetrics_.diskDrives[i].driveName, sizeof(s.name) - 1);
            s.name[sizeof(s.name) - 1] = '\0';
            s.freeSpacePercent =
                static_cast<int>(pcMetrics_.diskDrives[i].freeSpacePercent + 0.5f);
            snapshot.push_back(s);
        }
    }  // mutex released here — all remaining work is lock-free

    if (snapshot.empty())
        return;

    const bool needsCreation =
        diskDriveWidgets_.empty() || (diskDriveWidgets_.size() != snapshot.size());

    if (!needsCreation)
        return;

    // Rebuild widgets from the snapshot
    diskDriveWidgets_.clear();

    const uint16_t guidelineY4 = 120;
    const uint16_t guidelineY5 = 150;
    const uint16_t height = guidelineY5 - guidelineY4;
    const uint16_t maxWidgetWidth = 120;
    uint16_t widgetWidth =
        static_cast<uint16_t>(kScreenWidth / snapshot.size());
    if (widgetWidth > maxWidgetWidth)
        widgetWidth = maxWidgetWidth;

    for (size_t i = 0; i < snapshot.size(); ++i) {
        uint16_t xPos = static_cast<uint16_t>(i * widgetWidth);

        auto w = MetricWidget::Builder(
                     WidgetInterface::Dimensions{xPos, guidelineY4, widgetWidth, height},
                     updateIntervalMs_)
                     .unit("%")
                     .range(0, 100)
                     .colorThresholds(0.0f, 95.0f)
                     .reverseThresholds(true)
                     .useDimColors(true)
                     .label(snapshot[i].name)
                     .labelWidth(kFanLabelWidth)
                     .value(snapshot[i].freeSpacePercent)
                     .build();

        if (w) {
            if (!w->isInitialized())
                w->initialize(context_);
            w->drawStatic();
            w->forceRefresh();
            w->draw(true);
            diskDriveWidgets_.push_back(std::move(w));
        }
    }

    if (getLogger()) {
        getLogger()->debugf("Created %d disk drive widgets", diskDriveWidgets_.size());
    }
}

void PcMetricsWidget::updateDiskDriveWidgets() {
    PcMetricsDiskLock lock(pcMetrics_);  // protect diskDrives for the duration of the update
    size_t updateCount = (pcMetrics_.diskDrives.size() > kMaxDiskWidgets) ? kMaxDiskWidgets : pcMetrics_.diskDrives.size();

    for (size_t i = 0; i < updateCount && i < diskDriveWidgets_.size(); i++) {
        const auto& drive = pcMetrics_.diskDrives[i];
        if (diskDriveWidgets_[i]) {
            int freeSpacePercent = static_cast<int>(drive.freeSpacePercent + 0.5f);
            int currentValue = diskDriveWidgets_[i]->getValue();

            // Only update and draw if value changed
            if (currentValue != freeSpacePercent) {
                diskDriveWidgets_[i]->setValue(freeSpacePercent);
                // Don't force redraw here - let the normal draw cycle handle it
            }

            // Always ensure the widget is drawn if it's initialized
            if (diskDriveWidgets_[i]->isInitialized()) {
                diskDriveWidgets_[i]->draw(false);
            }
        }
    }
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

                // FORCE INITIAL VALUE DRAWING - even for value 0
                widget->forceRefresh();  // Reset internal state
                widget->draw(true);      // Force immediate redraw with current value
            }
        };

        // CPU widgets
        initAndDrawStatic(cpuLoadWidget_);
        initAndDrawStatic(cpuTemperatureWidget_);
        initAndDrawStatic(cpuPowerWidget_);
        initAndDrawStatic(cpuFanWidget_);

        // GPU widgets - especially important for gpuComputeWidget_ with value 0
        initAndDrawStatic(gpuLoadWidget_);
        initAndDrawStatic(gpuPowerWidget_);
        initAndDrawStatic(gpuTemperatureWidget_);
        initAndDrawStatic(gpu3dWidget_);
        initAndDrawStatic(gpuComputeWidget_);  // This one has value 0
        initAndDrawStatic(gpuFanWidget_);
        initAndDrawStatic(gpuMemoryWidget_);

        // Memory widget
        initAndDrawStatic(memoryLoadWidget_);

        // System fan widgets
        initAndDrawStatic(fanWidget1_);
        initAndDrawStatic(fanWidget2_);

        // Disk drive widgets - force immediate display
        ensureDiskWidgetsCreated();
        for (auto& driveWidget : diskDriveWidgets_) {
            if (driveWidget) {
                driveWidget->initialize(context_);
                driveWidget->drawStatic();
                driveWidget->forceRefresh();  // Reset internal state
                driveWidget->draw(true);      // Force immediate display
            }
        }

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
        } else {
            restoreStaticDisplay();
        }
        wasFreshData_ = currentlyHasFreshData;
    }

    // Ensure disk widgets are created whenever we have data
    if (currentlyHasFreshData) {
        ensureDiskWidgetsCreated();
    }

    // Only update dynamic content if we have fresh data and need to redraw
    bool needsRedraw = forceRedraw || isDirty() || needsUpdate();
    if (currentlyHasFreshData && needsRedraw) {
        drawDynamicData();
        clearDirty();  // Clear parent dirty flag BEFORE drawing children
    }

    lastUpdateTimeMs_ = millis();
}

void PcMetricsWidget::drawDynamicData() {
    // Only draw dynamic data if static elements are already drawn
    if (!isStaticDrawn_) {
        return;
    }

    // Helper lambda - update and draw all widgets, even with value 0
    auto updateAndDraw = [](const std::unique_ptr<MetricWidget>& widget, float value) {
        if (widget) {
            int intValue = static_cast<int>(value);
            // Always update and draw, don't check for changes
            widget->setValue(intValue);
            widget->draw(false);
        }
    };

    // CPU widgets
    updateAndDraw(cpuLoadWidget_, pcMetrics_.cpu_load);
    updateAndDraw(cpuTemperatureWidget_, pcMetrics_.cpu_temperature);
    updateAndDraw(cpuPowerWidget_, pcMetrics_.cpu_power);
    updateAndDraw(cpuFanWidget_, pcMetrics_.cpu_fan);

    // GPU widgets - this will fix gpuComputeWidget_ with value 0
    updateAndDraw(gpuLoadWidget_, pcMetrics_.gpu_load);
    updateAndDraw(gpuTemperatureWidget_, pcMetrics_.gpu_temperature);
    updateAndDraw(gpuPowerWidget_, pcMetrics_.gpu_power);
    updateAndDraw(gpu3dWidget_, pcMetrics_.gpu_3d);
    updateAndDraw(gpuComputeWidget_, pcMetrics_.gpu_compute);  // This should now show value 0
    updateAndDraw(gpuMemoryWidget_, pcMetrics_.gpu_mem);
    updateAndDraw(gpuFanWidget_, pcMetrics_.gpu_fan);

    // Memory widget
    updateAndDraw(memoryLoadWidget_, pcMetrics_.mem_load);

    // System fan widgets
    if (fanWidget1_) {
        fanWidget1_->setValue(pcMetrics_.system_fans[kSystemFan1Index]);
        fanWidget1_->draw(false);
    }
    if (fanWidget2_) {
        fanWidget2_->setValue(pcMetrics_.system_fans[kSystemFan2Index]);
        fanWidget2_->draw(false);
    }

    // Disk drive widgets
    updateDiskDriveWidgets();

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

    // ADD CLEARING DISK DRIVE WIDGETS
    for (auto& driveWidget : diskDriveWidgets_) {
        clearWidget(driveWidget);
    }

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