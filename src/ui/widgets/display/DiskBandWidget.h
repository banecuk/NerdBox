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
// free-space tiles, each with a solid 4px write line above and read line below
// colour-coded by activity rate. The whole band is tappable (default
// SHOW_DISKS action, mirroring FpsWidget).
//
// Extracted from PcMetricsWidget so it can be placed anywhere on a screen. All
// tile/line positions derive from dimensions_ rather than hardcoded absolute
// pixels, so the strip renders correctly at any origin. The tiles render
// borderless (MetricWidget::setBorderMargin(0)) so their coloured area runs
// flush against both activity lines — no wasted pixels at the strip edges.
//
// Strip anatomy (top to bottom):
//   kWriteLineY            → 4px read-rate line
//   kDiskAreaY             → per-drive MetricWidget tiles (borderless, fill
//                            the space down to the read line)
//   kReadLineHeight        → 4px write-rate line (flush to widget bottom)
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
    // Activity-line heights (px) — read/write activity monitors. Doubled from
    // 2px to 4px so the activity state is more visible.
    static constexpr uint16_t kWriteLineHeight = 4;
    static constexpr uint16_t kReadLineHeight = 4;

    // Horizontal line positions (vertical offsets from the widget's origin).
    static constexpr uint16_t kWriteLineY = 0;
    // Tile area top: the write line. The tile's own border is disabled
    // (borderMargin 0), so the tile fill starts immediately below the line —
    // no explicit gap needed between them.
    static constexpr uint16_t kDiskAreaY = kWriteLineY + kWriteLineHeight;

    // Maximum number of disk-drive tiles that can be displayed simultaneously
    static constexpr size_t kMaxDiskWidgets = 10;

    // Maximum single-drive tile width; on narrower multipliers drives are inset.
    static constexpr uint16_t kMaxWidgetWidth = 120;

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

    // Read-line vertical offset within the strip (relative to the widget's
    // origin); the tile area fills between kDiskAreaY and this row.
    uint16_t readLineYRelative() const { return dimensions_.height - kReadLineHeight; }
};
