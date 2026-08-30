#include "PcMetricsWidget.h"

#include <cmath>

#include "config/PcMetricsTilesConfig.h"
#include "ui/core/Colors.h"
#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

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
float valueSwapUsed(const PcMetrics& m) {
    // drawDynamicData() truncates with static_cast<int>, so round here:
    // 2.6 GB must display as 3, not 2.
    return roundf(m.mem_swap_used_gb);
}
}  // namespace

PcMetricsWidget::PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                                 uint32_t updateIntervalMs, PcMetrics& pcMetrics, EventType action,
                                 ActionCallback callback)
    : PcDataCompositeWidget(dims, updateIntervalMs, pcMetrics),
      action_(action),
      callback_(std::move(callback)) {
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
    // are ordered CPU row, RAM, GPU row, VRAM, 3D/compute/swap. The swap tile
    // sits at kCol4 (not kCol2) — the fan slots take col2/col3 on row 3.
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
        {kCol4, kRow3, kTileWidth, kRowH},
    };
    static const std::array<float (*)(const PcMetrics&), kFixedTileCount> kGetters = {
        valueCpuLoad, valueCpuTemperature, valueCpuPower, valueCpuFan, valueMemoryLoad,
        valueGpuLoad, valueGpuTemperature, valueGpuPower, valueGpuFan, valueGpuMemory,
        valueGpu3d,   valueGpuCompute,     valueSwapUsed,
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
            config.borderMargin = 0;
            tiles[i] = {kDims[i], config, kGetters[i]};
        }
        return tiles;
    }();
    return kTiles;
}

void PcMetricsWidget::buildFixedWidgets() {
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        const FixedTileDescriptor& d = fixedTileDescriptors()[i];
        fixedWidgets_[i] =
            std::make_unique<MetricWidget>(toScreenSpace(d.dims), updateIntervalMs_, d.config);
    }
}

void PcMetricsWidget::ensureChildWidgetsCreated() {
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

    // Two fixed slots at row 3, cols 2-3; col4 is the swap tile.
    static constexpr uint16_t kFanX[kMaxSystemFanWidgets] = {kCol2, kCol3};
    static constexpr const char* kFanLabels[kMaxSystemFanWidgets] = {"R1", "F1"};
    const uint8_t slots =
        min(static_cast<uint8_t>(fanCount), static_cast<uint8_t>(kMaxSystemFanWidgets));

    for (uint8_t i = 0; i < slots; ++i) {
        MetricWidget::Config config;
        config.unit = "";
        config.maxValue = 1500;
        config.lowerThreshold = 750.0f;
        config.upperThreshold = 1200.0f;
        config.label = kFanLabels[i];
        config.labelWidth = kLabelWidth;
        config.labelColor = 0xC618;
        config.useDimColors = true;
        config.borderMargin = 0;

        auto w = std::make_unique<MetricWidget>(
            toScreenSpace(WidgetInterface::Dimensions{kFanX[i], kRow3, kTileWidth, kRowH}),
            updateIntervalMs_, config);

        if (w) {
            initAndDrawWidget(*w);
            systemFanWidgets_.push_back(std::move(w));
        }
    }
}

void PcMetricsWidget::drawFreshStatic() {
    for (auto& w : fixedWidgets_) {
        if (w)
            initAndDrawWidget(*w);
    }
    ensureChildWidgetsCreated();
    for (auto& fw : systemFanWidgets_) {
        if (fw)
            initAndDrawWidget(*fw);
    }
}

void PcMetricsWidget::drawDynamicData() {
    if (!isStaticDrawn_)
        return;

    // A new sample arrived (rather than this being a mid-cycle interpolation
    // wake-up) exactly when the shared freshness timestamp is about to move —
    // lastUpdateTimestamp_ still holds the previous value here; the caller
    // (PcDataCompositeWidget::onDraw) only updates it after this call returns.
    const bool newSampleArrived = pcMetrics_.freshness.lastUpdateMs() != lastUpdateTimestamp_;
    if (newSampleArrived) {
        cpuLoadInterpolator_.onSample(valueCpuLoad(pcMetrics_), millis());
    }

    LGFX* lcd = getLcd();

    Fonts::loadMetric(lcd);
    const auto& descriptors = fixedTileDescriptors();
    for (uint8_t i = 0; i < kFixedTileCount; ++i) {
        auto& widget = fixedWidgets_[i];
        if (!widget)
            continue;
        const float raw = (i == kCpuLoad) ? cpuLoadInterpolator_.displayValue(millis())
                                          : descriptors[i].getValue(pcMetrics_);
        widget->setValue(static_cast<int>(raw));
        widget->drawValueWithLoadedFont();
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
}

void PcMetricsWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

void PcMetricsWidget::clearChildren() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);
    systemFanWidgets_.clear();
    lastSystemFanCount_ = 0xFF;
    // Restart the interpolation cycle instead of averaging against a raw
    // CPU-load sample from before the stale/fresh gap.
    cpuLoadInterpolator_ = MidpointInterpolator();
}

bool PcMetricsWidget::needsUpdate() const {
    if (PcDataCompositeWidget::needsUpdate())
        return true;
    // One extra wake-up per cycle, right at the interpolator's midpoint
    // deadline, to snap the CPU tile from the averaged value to the actual
    // sample — see MidpointInterpolator. consumeRevealDue() is one-shot, so
    // this doesn't degrade into a continuous time-elapsed poll once the
    // deadline has passed (see 07-performance.md P0-3).
    return hasFreshData() && cpuLoadInterpolator_.consumeRevealDue(millis());
}

bool PcMetricsWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!callback_)
        return false;

    for (uint8_t i = kGpuTileFirstIndex; i <= kGpuTileLastIndex; ++i) {
        const auto& widget = fixedWidgets_[i];
        if (!widget)
            continue;
        const auto d = widget->getDimensions();
        if (x >= d.x && x < d.x + d.width && y >= d.y && y < d.y + d.height) {
            callback_(action_);
            return true;
        }
    }
    return false;
}
