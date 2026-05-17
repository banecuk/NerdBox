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
      systemMetrics_(systemMetrics),
      freshnessGuard_(pcMetrics) {
    buildCpuWidgets();
    buildGpuWidgets();
    buildMemoryWidget();
    // System fan widgets are built lazily in ensureSystemFanWidgetsCreated()
    // once the first data fetch reveals how many fans are actually connected.
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
            .labelColor(0xC618)
            .build();

    cpuTemperatureWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow1, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("\xC2\xB0""C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(kLabelWidth)
            .labelColor(0xC618)
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
            .labelColor(0xC618)
            .build();

    cpuFanWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow2, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1200.0f)
            .label("FAN")
            .labelWidth(kLabelWidth)
            .labelColor(0xC618)
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
            .labelColor(0xAD27)
            .useGpuColors()
            .build();

    gpuTemperatureWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow3, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("\xC2\xB0""C")
            .range(0, 100)
            .colorThresholds(55.0f, 85.0f)
            .label("TMP")
            .labelWidth(kLabelWidth)
            .labelColor(0xAD27)
            .useGpuColors()
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
            .labelColor(0xAD27)
            .useGpuColors()
            .build();

    // Row 3–4 span: GPU memory (double height)
    gpuMemoryWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol6, kRow3, kTileWidth, kRowH * 2}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(30.0f, 90.0f)
            .label("VRM")
            .labelWidth(kLabelWidth)
            .labelColor(0xAD27)
            .useGpuColors()
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
            .labelColor(0xAD27)
            .useGpuColors()
            .build();

    gpuFanWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol9, kRow4, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("")
            .range(0, 1500)
            .colorThresholds(800.0f, 1400.0f)
            .label("FAN")
            .labelWidth(kLabelWidth)
            .labelColor(0xAD27)
            .useGpuColors()
            .build();

    gpuComputeWidget_ =
        MetricWidget::Builder(
            WidgetInterface::Dimensions{kCol7, kRow4, kTileWidth, kRowH}, updateIntervalMs_)
            .unit("%")
            .range(0, 100)
            .colorThresholds(10.0f, 90.0f)
            .label("CMP")
            .labelWidth(kLabelWidth)
            .labelColor(0xAD27)
            .useGpuColors()
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
            .labelColor(0xC618)
            .build();
}

void PcMetricsWidget::ensureSystemFanWidgetsCreated() {
    const uint8_t fanCount = pcMetrics_.system_fan_count;

    // Nothing to do if the count hasn't changed since last call.
    if (fanCount == lastSystemFanCount_) {
        return;
    }

    // Clear old tiles from the display before rebuilding.
    LGFX* lcd = getLcd();
    if (lcd) {
        for (auto& w : systemFanWidgets_) {
            if (w) {
                auto d = w->getDimensions();
                lcd->fillRect(d.x, d.y, d.width, d.height, TFT_BLACK);
            }
        }
    }
    systemFanWidgets_.clear();

    lastSystemFanCount_ = fanCount;

    if (fanCount == 0) {
        return;
    }

    // System fans stack vertically in the left column starting at kRow3.
    // Each tile is one row tall; fans beyond the available rows are silently
    // capped by kMaxSystemFanWidgets.
    const uint16_t fanWidth = kCol5 - kColFan;
    const uint8_t  slots    = min(static_cast<uint8_t>(fanCount),
                                  static_cast<uint8_t>(kMaxSystemFanWidgets));

    // Pre-build label strings: "F1" … "F<n>"
    char label[4];
    for (uint8_t i = 0; i < slots; ++i) {
        snprintf(label, sizeof(label), "F%u", static_cast<unsigned>(i + 1));

        auto w = MetricWidget::Builder(
                     WidgetInterface::Dimensions{kColFan,
                                                static_cast<uint16_t>(kRow3 + i * kRowH),
                                                fanWidth, kRowH},
                     updateIntervalMs_)
                     .unit("")
                     .range(0, 1500)
                     .colorThresholds(750.0f, 1200.0f)
                     .label(label)
                     .labelWidth(kFanLabelWidth)
                     .labelColor(0xC618)
                     .smallFont()  // 4-digit RPM values don't fit NotoSans18 in the narrow fan tile
                     .build();

        if (w) {
            initAndDrawWidget(*w);
            systemFanWidgets_.push_back(std::move(w));
        }
    }

    if (getLogger()) {
        getLogger()->debugf("System fan widgets: %u tile(s) for %u fan(s)", slots, fanCount);
    }
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
                     .unit("")
                     .range(0, 100)
                     .colorThresholds(0.0f, 95.0f)
                     .reverseThresholds(true)
                     .useDimColors(true)
                     .label(snapshot[i].name)
                     .labelWidth(kFanLabelWidth)
                     .value(snapshot[i].freeSpacePercent)
                     .build();

        if (w) {
            if (!w->isInitialized()) w->initialize(context_);
            initAndDrawWidget(*w);
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

void PcMetricsWidget::initAndDrawWidget(MetricWidget& widget) {
    widget.initialize(context_);
    widget.drawStatic();
    widget.forceRefresh();
    widget.draw(true);
}

void PcMetricsWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    if (hasFreshData()) {
        initAndDrawWidget(*cpuLoadWidget_);
        initAndDrawWidget(*cpuTemperatureWidget_);
        initAndDrawWidget(*cpuPowerWidget_);
        initAndDrawWidget(*cpuFanWidget_);
        initAndDrawWidget(*gpuLoadWidget_);
        initAndDrawWidget(*gpuPowerWidget_);
        initAndDrawWidget(*gpuTemperatureWidget_);
        initAndDrawWidget(*gpu3dWidget_);
        initAndDrawWidget(*gpuComputeWidget_);
        initAndDrawWidget(*gpuFanWidget_);
        initAndDrawWidget(*gpuMemoryWidget_);
        initAndDrawWidget(*memoryLoadWidget_);

        // System fans — created/rebuilt based on live fan count.
        ensureSystemFanWidgetsCreated();
        for (auto& fw : systemFanWidgets_) {
            if (fw) initAndDrawWidget(*fw);
        }

        ensureDiskWidgetsCreated();
        for (auto& dw : diskDriveWidgets_) {
            if (dw) initAndDrawWidget(*dw);
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
            // Static chrome is now drawn; immediately render the first frame of
            // dynamic data so values appear without waiting for needsUpdate() to
            // fire on the next cycle.
            drawDynamicData();
            clearDirty();
        }
        wasFreshData_ = currentlyHasFreshData;
    }

    // Only rebuild fan/disk widgets when a new fetch has actually landed.
    // Comparing against pcMetrics_.last_update_timestamp avoids the mutex
    // acquire, snapshot allocation, and size comparison on every draw frame.
    if (currentlyHasFreshData &&
        pcMetrics_.last_update_timestamp != lastEnsureCheckTimestamp_) {
        ensureSystemFanWidgetsCreated();
        ensureDiskWidgetsCreated();
        lastEnsureCheckTimestamp_ = pcMetrics_.last_update_timestamp;
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
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count && i < systemFanWidgets_.size(); ++i) {
        if (systemFanWidgets_[i]) {
            systemFanWidgets_[i]->setValue(pcMetrics_.system_fans[i]);
            systemFanWidgets_[i]->drawValueWithLoadedFont();
        }
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
    // One fillRect over the entire widget area covers every child tile.
    // No per-child fills needed — they are all within dimensions_.
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);

    systemFanWidgets_.clear();
    lastSystemFanCount_ = 0xFF;  // force rebuild on next data arrive
    diskDriveWidgets_.clear();

    lastEnsureCheckTimestamp_ = 0;  // force both ensures to run after next fetch
    isStaticDrawn_ = false;
}

void PcMetricsWidget::restoreStaticDisplay() {
    // Clear everything and redraw static elements
    clearAllWidgets();

    // Draw static elements (this will only draw if fresh data is available)
    drawStatic();
}

bool PcMetricsWidget::needsUpdate() const {
    if (!isInitialized_) {
        return false;
    }
    // Signal a redraw whenever the freshness state has changed (data arrived
    // or went stale) so onDraw() can handle the transition — even if static
    // chrome hasn't been drawn yet.
    if (hasFreshData() != wasFreshData_) {
        return true;
    }
    // Pure timestamp/interval query for steady-state refreshes.
    // onDraw() owns all wasFreshData_ transitions; we only ask whether new
    // data has landed or the periodic interval has elapsed.
    return (pcMetrics_.last_update_timestamp > lastUpdateTimestamp_) ||
           (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
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
