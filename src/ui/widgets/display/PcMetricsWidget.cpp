#include "PcMetricsWidget.h"

#include <cstdio>

#include "config/PcMetricsTilesConfig.h"
#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

namespace {
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
    static_assert(sizeof(PcMetricsTilesConfig::kTiles) / sizeof(PcMetricsTilesConfig::kTiles[0]) ==
                      kFixedTileCount,
                  "PcMetricsTilesConfig::kTiles must have one entry per FixedTile");

    // Layout (dims) and the PcMetrics field each tile reads (getValue) are
    // structural and live here; thresholds/colors/labels are data — see
    // config/PcMetricsTilesConfig.h. Position in each array must match: both
    // are ordered CPU row, RAM, GPU row, VRAM, 3D/compute.
    static const std::array<WidgetInterface::Dimensions, kFixedTileCount> kDims = {
        WidgetInterface::Dimensions{kCol0, kRow1, kTileWidth, kRowH},
        {kCol1, kRow1, kTileWidth, kRowH},
        {kCol2, kRow1, kTileWidth, kRowH},
        {kCol3, kRow1, kTileWidth, kRowH},
        {kCol4, kRow1, kTileWidth, kRowH},
        {kCol0, kRow2, kTileWidth, kRowH},
        {kCol1, kRow2, kTileWidth, kRowH},
        {kCol2, kRow2, kTileWidth, kRowH},
        {kCol3, kRow2, kTileWidth, kRowH},
        {kCol4, kRow2, kTileWidth, kRowH},
        {kCol0, kRow3, kTileWidth, kRowH},
        {kCol1, kRow3, kTileWidth, kRowH},
    };
    static const std::array<float (*)(const PcMetrics&), kFixedTileCount> kGetters = {
        valueCpuLoad,        valueCpuTemperature, valueCpuPower, valueCpuFan,
        valueMemoryLoad,     valueGpuLoad,        valueGpuTemperature,
        valueGpuPower,       valueGpuFan,         valueGpuMemory,
        valueGpu3d,          valueGpuCompute,
    };

    static const std::array<FixedTileDescriptor, kFixedTileCount> kTiles = [] {
        std::array<FixedTileDescriptor, kFixedTileCount> tiles{};
        for (uint8_t i = 0; i < kFixedTileCount; ++i) {
            const PcMetricsTilesConfig::TileData& t = PcMetricsTilesConfig::kTiles[i];
            MetricWidget::Config config;
            config.unit = t.unit;
            config.minValue = t.rangeMin;
            config.maxValue = t.rangeMax;
            config.lowerThreshold = t.thresholdLow;
            config.upperThreshold = t.thresholdHigh;
            config.label = t.label;
            config.labelWidth = kLabelWidth;
            config.labelColor = t.labelColor;
            config.useGpuColors = t.useGpuColors;
            config.useDimColors = t.useDimColors;
            config.useRamColors = t.useRamColors;
            tiles[i] = {kDims[i], config, kGetters[i]};
        }
        return tiles;
    }();
    return kTiles;
}

void PcMetricsWidget::buildFixedWidgets() {
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        const FixedTileDescriptor& d = fixedTileDescriptors()[i];
        fixedWidgets_[i] = std::make_unique<MetricWidget>(toScreenSpace(d.dims),
                                                          updateIntervalMs_, d.config);
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

        MetricWidget::Config config;
        config.unit = "";
        config.maxValue = 1500;
        config.lowerThreshold = 750.0f;
        config.upperThreshold = 1200.0f;
        config.label = label;
        config.labelWidth = kFanLabelWidth;
        config.labelColor = 0xC618;
        config.useDimColors = true;

        auto w = std::make_unique<MetricWidget>(
            toScreenSpace(WidgetInterface::Dimensions{kFanX[i], kRow3, kTileWidth, kRowH}),
            updateIntervalMs_, config);

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
