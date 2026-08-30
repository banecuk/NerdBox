#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <vector>

#include "config/AppSettings.h"
#include "core/events/EventTypes.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/PcDataCompositeWidget.h"
#include "ui/widgets/display/MetricWidget.h"
#include "utils/MidpointInterpolator.h"

// Composite CPU + GPU + RAM/VRAM tile grid plus up to two system-fan tiles,
// in a 5-column x 3-row, 96px-wide grid. Shared by MainScreen and GameScreen,
// both 84px tall (28px rows), via the rowHeight() rescale in toScreenSpace().
class PcMetricsWidget : public PcDataCompositeWidget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    // action/callback mirror AirQualityWidget/FpsWidget's tap-to-navigate
    // pattern: a tap on any GPU tile (load/temp/power/fan/memory/3D/compute)
    // publishes `action` via `callback` (e.g. to open the game screen).
    // No-op when either is left at its default.
    PcMetricsWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                    uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                    EventType action = EventType::NONE, ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

    bool needsUpdate() const override;

 protected:
    void drawFreshStatic() override;
    void drawDynamicData() override;
    void clearChildren() override;
    void ensureChildWidgetsCreated() override;
    void drawNoDataMessage() override;

 private:
    // Layout constants — tile columns/rows are relative offsets from this
    // widget's own origin (dims_.x / dims_.y), so the grid renders correctly
    // at any position. Absolute screen pixels are computed via toScreenSpace().
    // kRowH is the reference tile height for a 3-row grid at a 90px widget
    // height; rowHeight() rescales it from the widget's actual dimensions_, so
    // shorter instances (e.g. MainScreen's 84px) get tighter 28px rows.
    static constexpr uint16_t kTileWidth = 96;
    static constexpr uint16_t kRowH = 30;
    static constexpr uint8_t kRowCount = 3;
    // Minimum row height that keeps NotoSans18's 23px text background box
    // inside a borderMargin=0 tile — see docs-local/10-pcmetrics-tile-height.md.
    // Below this, per-glyph background fill bleeds into the neighbouring tile.
    static constexpr uint16_t kMinRowH = 24;
    static constexpr uint16_t kCol0 = 0;
    static constexpr uint16_t kCol1 = kTileWidth;
    static constexpr uint16_t kCol2 = kTileWidth * 2;
    static constexpr uint16_t kCol3 = kTileWidth * 3;
    static constexpr uint16_t kCol4 = kTileWidth * 4;
    static constexpr uint16_t kRow1 = 0;
    static constexpr uint16_t kRow2 = kRowH;
    static constexpr uint16_t kRow3 = 2 * kRowH;
    static constexpr uint8_t kLabelWidth = 26;
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
        kSwapUsed,
        kFixedTileCount
    };

    struct FixedTileDescriptor {
        WidgetInterface::Dimensions dims;
        MetricWidget::Config config;
        float (*getValue)(const PcMetrics&);
    };

    static const std::array<PcMetricsWidget::FixedTileDescriptor, PcMetricsWidget::kFixedTileCount>&
    fixedTileDescriptors();

    // fixedTileDescriptors()'s kGetters/kDims are laid out CPU row (indices
    // 0-4: load/temp/power/fan/RAM), then GPU row + GPU-adjacent row 3 tiles
    // (indices 5-12: load/temp/power/fan/memory/3D/compute/swap) — the
    // contiguous GPU range a tap check needs. FixedTile's own enum values are
    // just names; they don't index these arrays. The swap tile (index 12) is
    // not a GPU tile, so it sits outside the range, same as RAM (index 4).
    static constexpr uint8_t kGpuTileFirstIndex = 5;
    static constexpr uint8_t kGpuTileLastIndex = 11;

    EventType action_;
    ActionCallback callback_;

    uint8_t lastSystemFanCount_ = 0xFF;  // sentinel: force creation on first data

    std::array<std::unique_ptr<MetricWidget>, kFixedTileCount> fixedWidgets_;
    std::vector<std::unique_ptr<MetricWidget>> systemFanWidgets_;

    // CPU load moves much faster than the other tiles between fetches, so it
    // alone gets a mid-cycle interpolated redraw — see MidpointInterpolator.
    // needsUpdate() only peeks (isRevealDue()); drawDynamicData() is the sole
    // caller of the mutating consumeRevealDue() — see 07-performance.md P1-20.
    MidpointInterpolator cpuLoadInterpolator_;

    // Row height derived from this widget's actual height (3-row grid). At the
    // nominal 90px instance this equals kRowH (30px); a shorter instance yields
    // a proportionally shorter tile row so tiles never clip.
    uint16_t rowHeight() const { return dimensions_.height / kRowCount; }

    // Translates a tile position relative to this widget's origin into
    // absolute screen coordinates. Column offsets are added directly; row
    // offsets and tile heights are rescaled from the nominal kRowH reference
    // to this widget's actual rowHeight() so the grid always fills dimensions_.
    WidgetInterface::Dimensions toScreenSpace(const WidgetInterface::Dimensions& relative) const {
        const uint16_t rh = rowHeight();
        return {static_cast<uint16_t>(dimensions_.x + relative.x),
                static_cast<uint16_t>(dimensions_.y + (relative.y * rh) / kRowH), relative.width,
                rh};
    }

    void buildFixedWidgets();
};
