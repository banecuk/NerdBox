#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <vector>

#include "core/events/EventTypes.h"
#include "ui/widgets/display/MetricWidget.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/PcDataCompositeWidget.h"
#include "utils/ScopedLock.h"

// Slim, position-independent disk-band strip: a single row of per-drive
// free-space tiles, each with a single solid activity line flush to its top
// edge, split left/right into a read half (green) and a write half (red).
// The whole band is tappable (default SHOW_DISKS action, mirroring
// FpsWidget).
//
// Extracted from PcMetricsWidget so it can be placed anywhere on a screen. All
// tile/line positions derive from dimensions_ rather than hardcoded absolute
// pixels, so the strip renders correctly at any origin. The tiles render
// borderless (MetricWidget::setBorderMargin(0)).
//
// Strip anatomy (top to bottom):
//   kActivityLineHeight     → one 2px line per tile, flush to widget top,
//                            left half = read rate, right half = write rate
//   kActivityGap            → 1px gap so the activity line stays visually
//                            separate from the value below it
//   kDiskAreaY              → per-drive MetricWidget tiles, filling the rest
//                            of the widget's height
class DiskBandWidget : public PcDataCompositeWidget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    DiskBandWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                   uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                   EventType action = EventType::SHOW_DISKS, ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void drawFreshStatic() override;
    void drawDynamicData() override;
    void clearChildren() override;
    void ensureChildWidgetsCreated() override;

 private:
    // Height (px) of the single read/write activity line at the top of
    // each tile.
    static constexpr uint16_t kActivityLineHeight = 3;

    // Gap (px) between the activity line and the value tile below it.
    static constexpr uint16_t kActivityGap = 1;

    // Tile area top: below the activity line and its gap.
    static constexpr uint16_t kDiskAreaY = kActivityLineHeight + kActivityGap;

    // Maximum number of disk-drive tiles that can be displayed simultaneously
    static constexpr size_t kMaxDiskWidgets = 10;

    // Maximum single-drive tile width; on narrower multipliers drives are inset.
    static constexpr uint16_t kMaxWidgetWidth = 120;

    // Drive-letter label column width (mirrors the MetricWidget tile's own
    // config.labelWidth), plus MetricWidget::SEPARATOR_WIDTH (1px) between
    // its label and value areas. The activity line above each tile is split
    // to match: the read half spans this label column, the write half spans
    // the rest (the larger value-display area) — so read/write widths line
    // up with what's underneath them.
    static constexpr uint16_t kLabelWidth = 14;
    static constexpr uint16_t kLabelColumnWidth = kLabelWidth + 1;

    EventType action_;
    ActionCallback callback_;

    std::vector<std::unique_ptr<MetricWidget>> diskDriveWidgets_;
    std::vector<uint16_t> diskWriteLineColor_;
    std::vector<uint16_t> diskReadLineColor_;
    std::vector<float> diskFreeSpaceSmoothed_;
    // Drive letters the current tiles were built for — lets ensureChildWidgetsCreated()
    // detect a same-count drive swap (e.g. D: unplugged, E: appears) that a
    // count-only comparison would miss.
    std::vector<std::array<char, 4>> diskDriveNames_;

};
