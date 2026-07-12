#pragma once
#include <array>
#include <atomic>
#include <vector>

#include "MetricWidget.h"
#include "config/AppSettings.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Composite metrics grid for the game screen: CPU + GPU + RAM/VRAM tiles plus
// up to two system-fan tiles, in a 5-column x 3-row, 96px-wide tile grid.
// Modeled on PcMetricsWidget but without disk drives, and using a wider tile
// (96px vs 86px) since there's no left-side disk/fan column to reserve here.
class GameMetricsWidget : public Widget {
 public:
    GameMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                      uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

    void setStaleTimeout(unsigned long timeoutMs) { freshnessGuard_.setTimeout(timeoutMs); }

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // Absolute layout constants — this widget is always placed at
    // {0, 130, 480, 90} on the game screen, so tile positions are hardcoded
    // absolute pixels (matching PcMetricsWidget's own convention).
    static constexpr uint16_t kTileWidth = 96;
    static constexpr uint16_t kRowH = 30;
    static constexpr uint16_t kCol0 = 0;
    static constexpr uint16_t kCol1 = kTileWidth;
    static constexpr uint16_t kCol2 = kTileWidth * 2;
    static constexpr uint16_t kCol3 = kTileWidth * 3;
    static constexpr uint16_t kCol4 = kTileWidth * 4;
    static constexpr uint16_t kRow1 = 130;
    static constexpr uint16_t kRow2 = kRow1 + kRowH;
    static constexpr uint16_t kRow3 = kRow2 + kRowH;
    static constexpr uint8_t kLabelWidth = 26;
    static constexpr uint8_t kFanLabelWidth = 14;
    static constexpr uint8_t kMaxSystemFanWidgets = 2;

    enum FixedTile : uint8_t {
        kCpuLoad = 0,
        kCpuTemperature,
        kCpuPower,
        kCpuFan,
        kGpuLoad,
        kGpuTemperature,
        kGpuPower,
        kGpu3d,
        kGpuCompute,
        kGpuMemory,
        kGpuFan,
        kMemoryLoad,
        kFixedTileCount
    };

    struct FixedTileDescriptor {
        WidgetInterface::Dimensions dims;
        const char* unit;
        int rangeMin;
        int rangeMax;
        float thresholdLow;
        float thresholdHigh;
        const char* label;
        uint8_t labelWidth;
        uint16_t labelColor;
        bool useGpuColors;
        bool useDimColors;
        bool useRamColors;
        float (*getValue)(const PcMetrics&);
    };

    static const std::array<FixedTileDescriptor, kFixedTileCount>& fixedTileDescriptors();

    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    DataFreshnessGuard<std::atomic<bool>, unsigned long> freshnessGuard_;

    unsigned long lastUpdateTimestamp_ = 0;
    bool wasFreshData_ = false;
    uint8_t lastSystemFanCount_ = 0xFF;  // sentinel: force creation on first data

    std::array<std::unique_ptr<MetricWidget>, kFixedTileCount> fixedWidgets_;
    std::vector<std::unique_ptr<MetricWidget>> systemFanWidgets_;

    void buildFixedWidgets();
    void ensureSystemFanWidgetsCreated();
    void drawDynamicData();
    void drawNoDataMessage();
    void clearAllWidgets();
    void restoreStaticDisplay();
    void initAndDrawWidget(MetricWidget& widget);
    bool hasFreshData() const { return freshnessGuard_.isFresh(); }
};
