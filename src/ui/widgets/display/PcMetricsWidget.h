#pragma once
#include <array>
#include <string>

#include "config/AppConfigInterface.h"
#include "MetricWidget.h"
#include "services/pcMetrics/DataFreshnessGuard.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "ui/widgets/display/ThreadsWidget.h"

class PcMetricsWidget : public Widget {
 public:
    PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                    uint32_t updateIntervalMs, PcMetrics& pcMetrics, AppConfigInterface& config,
                    ApplicationMetrics& systemMetrics);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

    void setStaleTimeout(unsigned long timeoutMs) { freshnessGuard_.setTimeout(timeoutMs); }
    unsigned long getStaleTimeout() const { return freshnessGuard_.getTimeout(); }

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    // -----------------------------------------------------------------------
    // Layout constants — all magic numbers live here
    // -----------------------------------------------------------------------

    // Screen width the widget is designed for
    static constexpr uint16_t kScreenWidth = 480;

    // Uniform width of every metric tile
    static constexpr uint16_t kTileWidth = 86;

    // Width of the text label inside each tile (e.g. "CPU", "TMP")
    static constexpr uint8_t kLabelWidth = 26;

    // Narrow label for the system-fan tiles (shorter label text)
    static constexpr uint8_t kFanLabelWidth = 14;

    // Horizontal grid — counted from the right edge so tiles stay aligned
    // regardless of kScreenWidth changes.
    static constexpr uint16_t kColFan = 0;                            // system fans / disk area (left)
    static constexpr uint16_t kCol5   = kScreenWidth - kTileWidth * 5;
    static constexpr uint16_t kCol6   = kScreenWidth - kTileWidth * 4;
    static constexpr uint16_t kCol7   = kScreenWidth - kTileWidth * 3;
    static constexpr uint16_t kCol8   = kScreenWidth - kTileWidth * 2;
    static constexpr uint16_t kCol9   = kScreenWidth - kTileWidth;

    // Vertical row baselines (each row is 30 px tall)
    static constexpr uint16_t kRow1 = 0;
    static constexpr uint16_t kRow2 = 30;
    static constexpr uint16_t kRow3 = 60;
    static constexpr uint16_t kRow4 = 90;
    static constexpr uint16_t kRow5 = 120;
    static constexpr uint16_t kRow6 = 150;

    // Row height shorthand
    static constexpr uint16_t kRowH = 30;  // kRow(n+1) - kRow(n)

    // Maximum number of disk-drive tiles that can be displayed simultaneously
    static constexpr size_t kMaxDiskWidgets = 10;

    // Maximum number of system-fan tiles that can be displayed simultaneously.
    // Matches PcMetrics::kMaxSystemFans so the widget can always represent every
    // connected fan without further bounds checks.
    static constexpr uint8_t kMaxSystemFanWidgets = PcMetrics::kMaxSystemFans;

    // -----------------------------------------------------------------------
    // Fixed tile layout — descriptor table replaces one named unique_ptr
    // member per tile. Index order here also fixes the draw order used by
    // drawStatic()/drawDynamicData().
    // -----------------------------------------------------------------------
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
        bool verticalLabel;
        float (*getValue)(const PcMetrics&);
    };

    static const std::array<FixedTileDescriptor, kFixedTileCount>& fixedTileDescriptors();

    // -----------------------------------------------------------------------
    // Dependencies
    // -----------------------------------------------------------------------
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    AppConfigInterface& config_;
    ApplicationMetrics& systemMetrics_;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    unsigned long lastUpdateTimestamp_ = 0;
    unsigned long lastEnsureCheckTimestamp_ = 0;  // last pcMetrics_ timestamp seen by the ensure calls
    DataFreshnessGuard freshnessGuard_;
    bool wasFreshData_ = false;
    // isStaticDrawn_ is inherited (protected) from Widget — previously
    // shadowed by a same-named member here, which meant Widget::cleanUp()
    // reset the base copy while this class read/wrote a separate one, so a
    // cleanup/reinit cycle could leave this class's chrome-drawn flag stale.
    uint8_t lastSystemFanCount_ = 0xFF;  // sentinel: force creation on first data

    // -----------------------------------------------------------------------
    // Child widgets
    // -----------------------------------------------------------------------

    // CPU/GPU/RAM tiles — one entry per FixedTile, built from
    // fixedTileDescriptors() in the constructor.
    std::array<std::unique_ptr<MetricWidget>, kFixedTileCount> fixedWidgets_;

    // System fans — built dynamically in ensureSystemFanWidgetsCreated() once
    // the compacted fan count arrives from the first data fetch.
    std::vector<std::unique_ptr<MetricWidget>> systemFanWidgets_;

    // Disk drives (created dynamically when data first arrives)
    std::vector<std::unique_ptr<MetricWidget>> diskDriveWidgets_;

    // -----------------------------------------------------------------------
    // Construction helpers
    // -----------------------------------------------------------------------
    void buildFixedWidgets();

    // -----------------------------------------------------------------------
    // Runtime helpers
    // -----------------------------------------------------------------------
    void drawDynamicData();
    void drawNoDataMessage();
    void clearAllWidgets();
    void restoreStaticDisplay();

    bool hasFreshData() const { return freshnessGuard_.isFresh(); }

    // Creates/recreates system-fan tiles whenever the live fan count changes.
    // Called from onDraw() after each data fetch so the layout always reflects
    // the actual number of spinning fans reported by Libre Hardware Monitor.
    void ensureSystemFanWidgetsCreated();

    void ensureDiskWidgetsCreated();
    void updateDiskDriveWidgets();

    // Runs the standard initialize → drawStatic → forceRefresh → draw(true)
    // sequence that every MetricWidget needs on first paint.  Used by
    // drawStatic(), ensureSystemFanWidgetsCreated(), and ensureDiskWidgetsCreated()
    // so the four-step sequence lives in exactly one place.
    void initAndDrawWidget(MetricWidget& widget);
};
