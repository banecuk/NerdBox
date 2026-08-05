#pragma once
#include <atomic>
#include <functional>
#include <vector>

#include "core/events/EventTypes.h"
#include "MetricWidget.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Slim, position-independent disk-band strip: a single row of per-drive
// free-space tiles, each with a solid 2px write line above and read line below
// colour-coded by activity rate, plus a ">" chevron at the right edge marking
// the band tappable (default SHOW_DISKS action, mirroring FpsWidget).
//
// Extracted from PcMetricsWidget so it can be placed anywhere on a screen. All
// tile/line positions derive from dimensions_ rather than hardcoded absolute
// pixels, so the strip renders correctly at any origin.
//
// Strip anatomy (top to bottom):
//   kWriteLineY            → 2px read-rate line
//   kDiskGap               → 1px
//   kDiskAreaY             → per-drive MetricWidget tiles (fill remaining height)
//   kReadLineGap           → 1px
//   kReadLineHeight        → 2px write-rate line (flush to widget bottom)
class DiskBandWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    DiskBandWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                   uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                   EventType action = EventType::SHOW_DISKS, ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;
    bool needsUpdate() const override;

    void setStaleTimeout(unsigned long timeoutMs) { freshnessGuard_.setTimeout(timeoutMs); }

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // Activity-line heights (px)
    static constexpr uint16_t kWriteLineHeight = 2;
    static constexpr uint16_t kReadLineHeight = 2;

    // 1px gap between the write line and the tiles.
    static constexpr uint16_t kDiskGap = 1;

    // Horizontal line positions (vertical offsets from the widget's origin).
    static constexpr uint16_t kWriteLineY = 0;
    // Tile area top: write line + gap.
    static constexpr uint16_t kDiskAreaY = kWriteLineY + kWriteLineHeight + kDiskGap;

    // Maximum number of disk-drive tiles that can be displayed simultaneously
    static constexpr size_t kMaxDiskWidgets = 10;

    // Maximum single-drive tile width; on narrower multipliers drives are inset.
    static constexpr uint16_t kMaxWidgetWidth = 120;

    PcMetrics& pcMetrics_;
    EventType action_;
    ActionCallback callback_;
    DataFreshnessGuard<std::atomic<bool>, unsigned long> freshnessGuard_;

    unsigned long lastUpdateTimestamp_ = 0;
    unsigned long lastEnsureCheckTimestamp_ = 0;
    bool wasFreshData_ = false;

    std::vector<std::unique_ptr<MetricWidget>> diskDriveWidgets_;
    std::vector<uint16_t> diskWriteLineColor_;
    std::vector<uint16_t> diskReadLineColor_;
    std::vector<float> diskFreeSpaceSmoothed_;

    // Read-line vertical offset within the strip (relative to the widget's
    // origin); the tile area fills between kDiskAreaY and this row.
    uint16_t readLineYRelative() const { return dimensions_.height - kReadLineHeight; }

    // Creates/recreates the drive tiles when the drive set changes.
    void ensureDiskWidgetsCreated();
    void updateDiskDriveWidgets();
    // Draws the ">" chevron hint at the right edge of the band.
    void drawDiskChevron();
    void initAndDrawWidget(MetricWidget& widget);
    void clearDiskWidgets();

    bool hasFreshData() const { return freshnessGuard_.isFresh(); }
};
