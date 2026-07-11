#include "PcMetricsWidget.h"

#include <cstdio>

#include "core/resources/FontRegistry.h"

namespace {
constexpr const char* kDegreesC =
    "\xC2\xB0"
    "C";

float valueCpuLoad(const PcMetrics& m) {
    return m.cpu_load;
}
float valueCpuTemperature(const PcMetrics& m) {
    return m.cpu_temperature;
}
float valueCpuPower(const PcMetrics& m) {
    return m.cpu_power;
}
float valueCpuFan(const PcMetrics& m) {
    return m.cpu_fan;
}
float valueGpuLoad(const PcMetrics& m) {
    return m.gpu_load;
}
float valueGpuTemperature(const PcMetrics& m) {
    return m.gpu_temperature;
}
float valueGpuPower(const PcMetrics& m) {
    return m.gpu_power;
}
float valueGpu3d(const PcMetrics& m) {
    return m.gpu_3d;
}
float valueGpuCompute(const PcMetrics& m) {
    return m.gpu_compute;
}
float valueGpuMemory(const PcMetrics& m) {
    return m.gpu_mem;
}
float valueGpuFan(const PcMetrics& m) {
    return m.gpu_fan;
}
float valueMemoryLoad(const PcMetrics& m) {
    return m.mem_load;
}

// Averages the RGB565 channels of two colors to produce an intermediate shade.
constexpr uint16_t blendColor565(uint16_t a, uint16_t b) {
    const uint8_t r = static_cast<uint8_t>((((a >> 11) & 0x1F) + ((b >> 11) & 0x1F)) / 2);
    const uint8_t g = static_cast<uint8_t>((((a >> 5) & 0x3F) + ((b >> 5) & 0x3F)) / 2);
    const uint8_t bl = static_cast<uint8_t>(((a & 0x1F) + (b & 0x1F)) / 2);
    return static_cast<uint16_t>((r << 11) | (g << 5) | bl);
}

constexpr uint16_t kDiskIdleColor = 0x2104;  // dark grey, matches other widgets' dim color

// Disk read/write activity color scale, in KB/s. Breakpoints: <2 MB/s dark
// gray (idle), 2-25 MB/s dark green, 25-50 MB/s light green, 50-75.5 MB/s
// yellow, >75.5 MB/s orange (capped -- everything above kSaturated stays
// orange), with a blended intermediate shade inserted at the midpoint of
// each band below kSaturated for finer gradation.
uint16_t diskActivityColor(float kbPerSec) {
    constexpr float kIdle = 2.0f * 1024.0f;
    constexpr float kIdleModerateMid = 13.5f * 1024.0f;
    constexpr float kModerate = 25.0f * 1024.0f;
    constexpr float kModerateHighMid = 37.5f * 1024.0f;
    constexpr float kHigh = 50.0f * 1024.0f;
    constexpr float kHighElevatedMid = 62.75f * 1024.0f;
    constexpr float kElevated = 75.5f * 1024.0f;
    constexpr float kElevatedSaturatedMid = 87.75f * 1024.0f;
    constexpr float kSaturated = 100.0f * 1024.0f;

    if (kbPerSec < kIdle)
        return kDiskIdleColor;
    if (kbPerSec < kIdleModerateMid)
        return blendColor565(kDiskIdleColor, TFT_DARKGREEN);
    if (kbPerSec < kModerate)
        return TFT_DARKGREEN;
    if (kbPerSec < kModerateHighMid)
        return blendColor565(TFT_DARKGREEN, TFT_GREEN);
    if (kbPerSec < kHigh)
        return TFT_GREEN;
    if (kbPerSec < kHighElevatedMid)
        return blendColor565(TFT_GREEN, TFT_YELLOW);
    if (kbPerSec < kElevated)
        return TFT_YELLOW;
    if (kbPerSec < kElevatedSaturatedMid)
        return blendColor565(TFT_YELLOW, TFT_ORANGE);
    return TFT_ORANGE;
}
}  // namespace

// ---------------------------------------------------------------------------
// Constructor — builds the fixed tiles from the descriptor table
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
    buildFixedWidgets();
    // System fan widgets are built lazily in ensureSystemFanWidgetsCreated()
    // once the first data fetch reveals how many fans are actually connected.
}

// ---------------------------------------------------------------------------
// Widget construction helpers
// ---------------------------------------------------------------------------

const std::array<PcMetricsWidget::FixedTileDescriptor, PcMetricsWidget::kFixedTileCount>&
PcMetricsWidget::fixedTileDescriptors() {
    static const std::array<FixedTileDescriptor, kFixedTileCount> kTiles = {
        {
         // CPU row
            {{kCol8, kRow1, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "CPU",
             kLabelWidth,
             0xC618,
             false,
             false,
             valueCpuLoad},
         {{kCol9, kRow1, kTileWidth, kRowH},
             kDegreesC,
             0,
             100,
             55.0f,
             85.0f,
             "TMP",
             kLabelWidth,
             0xC618,
             false,
             false,
             valueCpuTemperature},
         {{kCol8, kRow2, kTileWidth, kRowH},
             " W",
             0,
             400,
             55.0f,
             140.0f,
             "PWR",
             kLabelWidth,
             0xC618,
             false,
             false,
             valueCpuPower},
         {{kCol9, kRow2, kTileWidth, kRowH},
             "",
             0,
             1500,
             800.0f,
             1200.0f,
             "FAN",
             kLabelWidth,
             0xC618,
             false,
             false,
             valueCpuFan},

         // GPU rows
            {{kCol8, kRow3, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "GPU",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpuLoad},
         {{kCol9, kRow3, kTileWidth, kRowH},
             kDegreesC,
             0,
             100,
             55.0f,
             85.0f,
             "TMP",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpuTemperature},
         {{kCol8, kRow4, kTileWidth, kRowH},
             " W",
             0,
             400,
             50.0f,
             170.0f,
             "PWR",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpuPower},
         {{kCol7, kRow3, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "3D",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpu3d},
         {{kCol7, kRow4, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "CMP",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpuCompute},
         {{kCol6, kRow3, kTileWidth, static_cast<uint16_t>(kRowH * 2)},
             "%",
             0,
             100,
             30.0f,
             90.0f,
             "VRAM",
             kLabelWidth,
             0xAD27,
             true,
             true,
             valueGpuMemory},
         {{kCol9, kRow4, kTileWidth, kRowH},
             "",
             0,
             1500,
             800.0f,
             1400.0f,
             "FAN",
             kLabelWidth,
             0xAD27,
             true,
             false,
             valueGpuFan},

         // RAM — rows 3-4 span (double height), leftmost of the right-side tiles
            {{kCol5, kRow3, kTileWidth, static_cast<uint16_t>(kRowH * 2)},
             "%",
             0,
             100,
             60.0f,
             90.0f,
             "RAM",
             kLabelWidth,
             0xC618,
             false,
             true,
             valueMemoryLoad},
         }
    };
    return kTiles;
}

void PcMetricsWidget::buildFixedWidgets() {
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        const FixedTileDescriptor& d = fixedTileDescriptors()[i];
        fixedWidgets_[i] = MetricWidget::Builder(d.dims, updateIntervalMs_)
                               .unit(d.unit)
                               .range(d.rangeMin, d.rangeMax)
                               .colorThresholds(d.thresholdLow, d.thresholdHigh)
                               .label(d.label)
                               .labelWidth(d.labelWidth)
                               .labelColor(d.labelColor)
                               .useGpuColors(d.useGpuColors)
                               .verticalLabel(d.verticalLabel)
                               .build();
    }
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
    const uint8_t slots =
        min(static_cast<uint8_t>(fanCount), static_cast<uint8_t>(kMaxSystemFanWidgets));

    // Pre-build label strings: "F1" … "F<n>"
    char label[4];
    for (uint8_t i = 0; i < slots; ++i) {
        snprintf(label, sizeof(label), "F%u", static_cast<unsigned>(i + 1));

        auto w = MetricWidget::Builder(
                     WidgetInterface::Dimensions{kColFan, static_cast<uint16_t>(kRow3 + i * kRowH),
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
        const size_t driveCount = pcMetrics_.disk_drives.size() < kMaxDiskWidgets
                                      ? pcMetrics_.disk_drives.size()
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
    diskWriteLineColor_.assign(snapshot.size(), 0xFFFF);  // sentinel forces first draw
    diskReadLineColor_.assign(snapshot.size(), 0xFFFF);
    diskFreeSpaceSmoothed_.assign(snapshot.size(), -1.0f);  // sentinel: no previous value yet

    const uint16_t maxWidgetWidth = 120;
    uint16_t widgetWidth = static_cast<uint16_t>(kScreenWidth / snapshot.size());
    if (widgetWidth > maxWidgetWidth)
        widgetWidth = maxWidgetWidth;

    for (size_t i = 0; i < snapshot.size(); ++i) {
        uint16_t xPos = static_cast<uint16_t>(i * widgetWidth);

        auto w = MetricWidget::Builder(
                     WidgetInterface::Dimensions{xPos, kDiskAreaY, widgetWidth, kDiskAreaHeight},
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
            if (!w->isInitialized())
                w->initialize(context_);
            initAndDrawWidget(*w);
            diskDriveWidgets_.push_back(std::move(w));
        }
    }

    if (getLogger()) {
        getLogger()->debugf("Created %d disk drive widgets", diskDriveWidgets_.size());
    }
}

void PcMetricsWidget::updateDiskDriveWidgets() {
    // Snapshot the free-space values under the lock, then release it before
    // touching the display.  Holding PcMetricsDiskLock across draw() would
    // keep the mutex taken while rendering pixels — a priority inversion risk
    // that can block the background fetch task and cause missed frames.
    // ensureDiskWidgetsCreated() uses the same snapshot pattern.
    const size_t widgetCount = diskDriveWidgets_.size();
    if (widgetCount == 0)
        return;

    // One float per widget slot (raw, pre-smoothing).
    float freeSpaceRawSnapshot[kMaxDiskWidgets];
    float writeSnapshot[kMaxDiskWidgets];
    float readSnapshot[kMaxDiskWidgets];
    size_t updateCount = 0;
    {
        PcMetricsDiskLock lock(pcMetrics_);
        updateCount = (pcMetrics_.disk_drives.size() < widgetCount) ? pcMetrics_.disk_drives.size()
                                                                    : widgetCount;
        for (size_t i = 0; i < updateCount; ++i) {
            freeSpaceRawSnapshot[i] = pcMetrics_.disk_drives[i].freeSpacePercent;
            writeSnapshot[i] = pcMetrics_.disk_drives[i].writeKBPerSec;
            readSnapshot[i] = pcMetrics_.disk_drives[i].readKBPerSec;
        }
    }  // mutex released — all display work below is lock-free

    for (size_t i = 0; i < updateCount; ++i) {
        if (!diskDriveWidgets_[i] || !diskDriveWidgets_[i]->isInitialized())
            continue;

        // Smooth falling free-space values: if the new reading is lower than
        // the previously displayed value, show the average of the two instead
        // of jumping straight down, and carry that average forward as the new
        // previous value. Rising values (or the first sample) are shown as-is.
        const float raw = freeSpaceRawSnapshot[i];
        const float previous = (i < diskFreeSpaceSmoothed_.size()) ? diskFreeSpaceSmoothed_[i] : -1.0f;
        const float smoothed = (previous >= 0.0f && raw < previous) ? (previous + raw) / 2.0f : raw;
        if (i < diskFreeSpaceSmoothed_.size())
            diskFreeSpaceSmoothed_[i] = smoothed;
        const int freeSpaceValue = static_cast<int>(smoothed + 0.5f);

        if (diskDriveWidgets_[i]->getValue() != freeSpaceValue) {
            diskDriveWidgets_[i]->setValue(freeSpaceValue);
        }
        diskDriveWidgets_[i]->draw(false);

        const auto dims = diskDriveWidgets_[i]->getDimensions();
        const uint16_t writeColor = diskActivityColor(writeSnapshot[i]);
        if (i >= diskWriteLineColor_.size())
            continue;
        if (diskWriteLineColor_[i] != writeColor) {
            getLcd()->fillRect(dims.x, kDiskWriteLineY, dims.width, kDiskActivityLineHeight,
                               writeColor);
            diskWriteLineColor_[i] = writeColor;
        }

        const uint16_t readColor = diskActivityColor(readSnapshot[i]);
        if (diskReadLineColor_[i] != readColor) {
            getLcd()->fillRect(dims.x, kDiskReadLineY, dims.width, kDiskActivityLineHeight,
                               readColor);
            diskReadLineColor_[i] = readColor;
        }
    }
}

void PcMetricsWidget::initAndDrawWidget(MetricWidget& widget) {
    // initialize() draws static chrome on first call only (no-op on repeat
    // calls once the widget is already initialized), so the explicit
    // drawStatic() below is what actually repaints chrome on every
    // subsequent stale->fresh transition. forceRefresh() resets the cached
    // layout state and performs the initial value draw immediately — a
    // trailing draw(true) would just repeat that same value draw.
    widget.initialize(context_);
    widget.drawStatic();
    widget.forceRefresh();
}

void PcMetricsWidget::onDrawStatic() {
    if (hasFreshData()) {
        for (auto& w : fixedWidgets_) {
            if (w)
                initAndDrawWidget(*w);
        }

        // System fans — created/rebuilt based on live fan count.
        ensureSystemFanWidgetsCreated();
        for (auto& fw : systemFanWidgets_) {
            if (fw)
                initAndDrawWidget(*fw);
        }

        ensureDiskWidgetsCreated();
        for (auto& dw : diskDriveWidgets_) {
            if (dw)
                initAndDrawWidget(*dw);
        }
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
    if (currentlyHasFreshData && pcMetrics_.last_update_timestamp != lastEnsureCheckTimestamp_) {
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

    // Two passes across the same batch of MetricWidgets, one font each,
    // instead of swapping fonts per-widget. loadFont() streams the font from
    // PROGMEM into RAM — expensive — so this keeps the cost at a flat 4
    // swaps (2 per pass) regardless of widget count, rather than 2 per widget
    // if each widget's dim unit suffix were drawn inline with its value.
    //
    // Pass 1 — values (NotoSans18). Each widget calls drawValueWithLoadedFont(),
    // which skips its own loadFont()/unloadFont() pair — see drawDynamicData's
    // original comment history: this alone took 28 heap alloc/free operations
    // down to 2 for the value pass.
    Fonts::loadMetric(lcd);

    const auto& descriptors = fixedTileDescriptors();
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        auto& widget = fixedWidgets_[i];
        if (widget) {
            widget->setValue(static_cast<int>(descriptors[i].getValue(pcMetrics_)));
            widget->drawValueWithLoadedFont();
        }
    }

    // System fans
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count && i < systemFanWidgets_.size(); ++i) {
        if (systemFanWidgets_[i]) {
            systemFanWidgets_[i]->setValue(pcMetrics_.system_fans[i]);
            systemFanWidgets_[i]->drawValueWithLoadedFont();
        }
    }

    Fonts::unload(lcd);

    // Pass 2 — unit suffixes, smaller font but same colour as the value
    // (NotoSansDisplay12). drawUnitWithLoadedFont() is a no-op unless the
    // paired value draw above actually moved or recoloured the unit, so
    // steady-state frames cost almost nothing here.
    Fonts::loadLabel(lcd);

    for (auto& widget : fixedWidgets_) {
        if (widget)
            widget->drawUnitWithLoadedFont();
    }

    for (uint8_t i = 0; i < pcMetrics_.system_fan_count && i < systemFanWidgets_.size(); ++i) {
        if (systemFanWidgets_[i]) {
            systemFanWidgets_[i]->drawUnitWithLoadedFont();
        }
    }

    Fonts::unload(lcd);

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
    diskWriteLineColor_.clear();
    diskReadLineColor_.clear();
    diskFreeSpaceSmoothed_.clear();

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
