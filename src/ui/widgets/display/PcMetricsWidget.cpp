#include "PcMetricsWidget.h"

#include <cstdio>

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

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
}  // namespace

PcMetricsWidget::PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                                 uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.freshness) {
    buildFixedWidgets();
}

const std::array<PcMetricsWidget::FixedTileDescriptor, PcMetricsWidget::kFixedTileCount>&
PcMetricsWidget::fixedTileDescriptors() {
    static const std::array<FixedTileDescriptor, kFixedTileCount> kTiles = {
        {
         // CPU row
            {{kCol0, kRow1, kTileWidth, kRowH},
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
             false,
             valueCpuLoad},
         {{kCol1, kRow1, kTileWidth, kRowH},
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
             false,
             valueCpuTemperature},
         {{kCol2, kRow1, kTileWidth, kRowH},
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
             false,
             valueCpuPower},
         {{kCol3, kRow1, kTileWidth, kRowH},
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
             false,
             valueCpuFan},

         // RAM — end of CPU row
            {{kCol4, kRow1, kTileWidth, kRowH},
             "%",
             0,
             100,
             60.0f,
             90.0f,
             "RAM",
             kLabelWidth,
             0xADFB,
             false,
             false,
             true,
             valueMemoryLoad},

         // GPU row
            {{kCol0, kRow2, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "GPU",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpuLoad},
         {{kCol1, kRow2, kTileWidth, kRowH},
             kDegreesC,
             0,
             100,
             55.0f,
             85.0f,
             "TMP",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpuTemperature},
         {{kCol2, kRow2, kTileWidth, kRowH},
             " W",
             0,
             400,
             50.0f,
             170.0f,
             "PWR",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpuPower},
         {{kCol3, kRow2, kTileWidth, kRowH},
             "",
             0,
             1500,
             800.0f,
             1400.0f,
             "FAN",
             kLabelWidth,
             0xB471,
             true,
             true,
             false,
             valueGpuFan},

         // VRAM — end of GPU row
            {{kCol4, kRow2, kTileWidth, kRowH},
             "%",
             0,
             100,
             30.0f,
             90.0f,
             "VRM",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpuMemory},

         // Row 3 — 3D / compute (fan slots kCol2/kCol3 are lazily created)
            {{kCol0, kRow3, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "3D",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpu3d},
         {{kCol1, kRow3, kTileWidth, kRowH},
             "%",
             0,
             100,
             10.0f,
             90.0f,
             "CMP",
             kLabelWidth,
             0xB471,
             true,
             false,
             false,
             valueGpuCompute},
         }
    };
    return kTiles;
}

void PcMetricsWidget::buildFixedWidgets() {
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        const FixedTileDescriptor& d = fixedTileDescriptors()[i];
        fixedWidgets_[i] = MetricWidget::Builder(toScreenSpace(d.dims), updateIntervalMs_)
                               .unit(d.unit)
                               .range(d.rangeMin, d.rangeMax)
                               .colorThresholds(d.thresholdLow, d.thresholdHigh)
                               .label(d.label)
                               .labelWidth(d.labelWidth)
                               .labelColor(d.labelColor)
                               .useGpuColors(d.useGpuColors)
                               .useDimColors(d.useDimColors)
                               .useRamColors(d.useRamColors)
                               .build();
    }
}

void PcMetricsWidget::ensureSystemFanWidgetsCreated() {
    const uint8_t fanCount = pcMetrics_.system_fan_count;
    if (fanCount == lastSystemFanCount_)
        return;

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

    if (fanCount == 0)
        return;

    // Two fixed slots at row 3, columns 2 and 3 (col 4 stays blank).
    static constexpr uint16_t kFanX[kMaxSystemFanWidgets] = {kCol2, kCol3};
    const uint8_t slots =
        min(static_cast<uint8_t>(fanCount), static_cast<uint8_t>(kMaxSystemFanWidgets));

    char label[4];
    for (uint8_t i = 0; i < slots; ++i) {
        snprintf(label, sizeof(label), "F%u", static_cast<unsigned>(i + 1));

        auto w = MetricWidget::Builder(
                     toScreenSpace(WidgetInterface::Dimensions{kFanX[i], kRow3, kTileWidth, kRowH}),
                     updateIntervalMs_)
                     .unit("")
                     .range(0, 1500)
                     .colorThresholds(750.0f, 1200.0f)
                     .label(label)
                     .labelWidth(kFanLabelWidth)
                     .labelColor(0xC618)
                     .useDimColors(true)
                     .build();

        if (w) {
            initAndDrawWidget(*w);
            systemFanWidgets_.push_back(std::move(w));
        }
    }
}

void PcMetricsWidget::initAndDrawWidget(MetricWidget& widget) {
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
        ensureSystemFanWidgetsCreated();
        for (auto& fw : systemFanWidgets_) {
            if (fw)
                initAndDrawWidget(*fw);
        }
    } else {
        clearAllWidgets();
        drawNoDataMessage();
    }
}

void PcMetricsWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool currentlyHasFreshData = hasFreshData();
    const bool stateChanged = (wasFreshData_ != currentlyHasFreshData);

    if (stateChanged) {
        if (!currentlyHasFreshData) {
            clearAllWidgets();
            drawNoDataMessage();
        } else {
            restoreStaticDisplay();
            drawDynamicData();
            clearDirty();
        }
        wasFreshData_ = currentlyHasFreshData;
    }

    if (currentlyHasFreshData && pcMetrics_.freshness.lastUpdateMs() != lastUpdateTimestamp_) {
        ensureSystemFanWidgetsCreated();
    }

    const bool needsRedraw = forceRedraw || isDirty() || needsUpdate();
    if (currentlyHasFreshData && needsRedraw) {
        drawDynamicData();
        clearDirty();
    }

    lastUpdateTimeMs_ = millis();
}

void PcMetricsWidget::drawDynamicData() {
    if (!isStaticDrawn_)
        return;

    LGFX* lcd = getLcd();

    Fonts::loadMetric(lcd);
    const auto& descriptors = fixedTileDescriptors();
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        auto& widget = fixedWidgets_[i];
        if (widget) {
            widget->setValue(static_cast<int>(descriptors[i].getValue(pcMetrics_)));
            widget->drawValueWithLoadedFont();
        }
    }
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count && i < systemFanWidgets_.size(); ++i) {
        if (systemFanWidgets_[i]) {
            systemFanWidgets_[i]->setValue(pcMetrics_.system_fans[i]);
            systemFanWidgets_[i]->drawValueWithLoadedFont();
        }
    }
    Fonts::unload(lcd);

    Fonts::loadLabel(lcd);
    for (auto& widget : fixedWidgets_) {
        if (widget)
            widget->drawUnitWithLoadedFont();
    }
    for (uint8_t i = 0; i < pcMetrics_.system_fan_count && i < systemFanWidgets_.size(); ++i) {
        if (systemFanWidgets_[i])
            systemFanWidgets_[i]->drawUnitWithLoadedFont();
    }
    Fonts::unload(lcd);

    lastUpdateTimestamp_ = pcMetrics_.freshness.lastUpdateMs();
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
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);
    systemFanWidgets_.clear();
    lastSystemFanCount_ = 0xFF;
    lastUpdateTimestamp_ = 0;
    isStaticDrawn_ = false;
}

void PcMetricsWidget::restoreStaticDisplay() {
    clearAllWidgets();
    drawStatic();
}

bool PcMetricsWidget::needsUpdate() const {
    if (!isInitialized_)
        return false;
    if (hasFreshData() != wasFreshData_)
        return true;
    return (pcMetrics_.freshness.lastUpdateMs() > lastUpdateTimestamp_) ||
           (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
