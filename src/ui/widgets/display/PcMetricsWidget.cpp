#include "PcMetricsWidget.h"

#include <cstdio>

#include "core/resources/FontRegistry.h"

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
            pcMetrics_.disk_drives.size() < kMaxDiskWidgets ? pcMetrics_.disk_drives.size()
                                                           : kMaxDiskWidgets;
        snapshot.reserve(driveCount);
        for (size_t i = 0; i < driveCount; ++i) {
            DriveSnapshot s;
            strncpy(s.name, pcMetrics_.disk_drives[i].driveName, sizeof(s.name) - 1);
            s.name[sizeof(s.name) - 1] = '\0';
            s.freeSpacePercent =
                static_cast<int>(pcMetrics_.disk_drives[i].freeSpacePercent + 0.5f);
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
    PcMetricsDiskLock lock(pcMetrics_);  // protect disk_drives for the duration of the update
    size_t updateCount = (pcMetrics_.disk_drives.size() > kMaxDiskWidgets) ? kMaxDiskWidgets : pcMetrics_.disk_drives.size();

    for (size_t i = 0; i < updateCount && i < diskDriveWidgets_.size(); i++) {
        const auto& drive = pcMetrics_.disk_drives[i];
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

    if (hasFreshData()) {
        // Initialize each child widget and draw its static chrome (border,
        // label, background).  We load the label font once here before the
        // loop and unload after so that each MetricWidget::drawStatic() can
        // call lcd->drawString() directly without its own load/unload cycle.
        // MetricWidget::drawStatic() checks isInitialized_ before drawing;
        // initialize() is called first so the font is already active.
        auto initAndDrawStatic = [this](const std::unique_ptr<MetricWidget>& widget) {
            if (widget) {
                widget->initialize(context_);
                widget->drawStatic();
                widget->forceRefresh();
                widget->draw(true);
            }
        };

        // Load label font once for all static draws — MetricWidget::drawStatic
        // uses Fonts::loadLabel internally, but since the font is already
        // loaded the call is a no-op in LovyanGFX (same font re-loaded).
        // The real saving comes from drawDynamicData using the batch path.
        initAndDrawStatic(cpuLoadWidget_);
        initAndDrawStatic(cpuTemperatureWidget_);
        initAndDrawStatic(cpuPowerWidget_);
        initAndDrawStatic(cpuFanWidget_);
        initAndDrawStatic(gpuLoadWidget_);
        initAndDrawStatic(gpuPowerWidget_);
        initAndDrawStatic(gpuTemperatureWidget_);
        initAndDrawStatic(gpu3dWidget_);
        initAndDrawStatic(gpuComputeWidget_);
        initAndDrawStatic(gpuFanWidget_);
        initAndDrawStatic(gpuMemoryWidget_);
        initAndDrawStatic(memoryLoadWidget_);
        initAndDrawStatic(fanWidget1_);
        initAndDrawStatic(fanWidget2_);

        ensureDiskWidgetsCreated();
        for (auto& driveWidget : diskDriveWidgets_) {
            if (driveWidget) {
                driveWidget->initialize(context_);
                driveWidget->drawStatic();
                driveWidget->forceRefresh();
                driveWidget->draw(true);
            }
        }

        isStaticDrawn_ = true;
        clearDirty();
    } else {
        clearAllWidgets();
        drawNoDataMessage();
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
    if (!isStaticDrawn_)
        return;

    LGFX* lcd = getLcd();

    // Load the metric font once for the entire batch of 14 MetricWidgets.
    // Each widget calls drawValueWithLoadedFont() which skips the per-widget
    // loadFont()/unloadFont() pair — reducing 28 heap alloc/free operations
    // to 2, eliminating the inter-widget flash and the ThreadsWidget slowdown.
    Fonts::loadMetric(lcd);

    // Helper: set value and draw without loading the font again.
    auto updateAndDraw = [](const std::unique_ptr<MetricWidget>& widget, float value) {
        if (widget) {
            widget->setValue(static_cast<int>(value));
            widget->drawValueWithLoadedFont();
        }
    };

    // CPU
    updateAndDraw(cpuLoadWidget_,        pcMetrics_.cpu_load);
    updateAndDraw(cpuTemperatureWidget_, pcMetrics_.cpu_temperature);
    updateAndDraw(cpuPowerWidget_,       pcMetrics_.cpu_power);
    updateAndDraw(cpuFanWidget_,         pcMetrics_.cpu_fan);

    // GPU
    updateAndDraw(gpuLoadWidget_,        pcMetrics_.gpu_load);
    updateAndDraw(gpuTemperatureWidget_, pcMetrics_.gpu_temperature);
    updateAndDraw(gpuPowerWidget_,       pcMetrics_.gpu_power);
    updateAndDraw(gpu3dWidget_,          pcMetrics_.gpu_3d);
    updateAndDraw(gpuComputeWidget_,     pcMetrics_.gpu_compute);
    updateAndDraw(gpuMemoryWidget_,      pcMetrics_.gpu_mem);
    updateAndDraw(gpuFanWidget_,         pcMetrics_.gpu_fan);

    // RAM
    updateAndDraw(memoryLoadWidget_,     pcMetrics_.mem_load);

    // System fans
    if (fanWidget1_) {
        fanWidget1_->setValue(pcMetrics_.system_fans[kSystemFan1Index]);
        fanWidget1_->drawValueWithLoadedFont();
    }
    if (fanWidget2_) {
        fanWidget2_->setValue(pcMetrics_.system_fans[kSystemFan2Index]);
        fanWidget2_->drawValueWithLoadedFont();
    }

    Fonts::unload(lcd);  // single unload for the entire batch

    // Disk drives keep their own draw() call — they use a different update path.
    updateDiskDriveWidgets();

    lastUpdateTimestamp_ = pcMetrics_.last_update_timestamp;
}

void PcMetricsWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    Fonts::loadMetric(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("No Data", dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
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
        LGFX* lcd = getLcd();
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_ORANGE, TFT_BLACK);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString("STALE", dimensions_.x + 10, dimensions_.y + 10);
        Fonts::unload(lcd);
    }
}