#pragma once

#include <cstddef>
#include <functional>

#include "core/events/EventTypes.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Compact disk activity summary — sits right of NetworkTrafficWidget in the
// bottom band. Two stacked rows, mirroring NetworkTrafficWidget's layout:
// write on top, read on bottom. Each row shows the sum of that direction's
// rate across every drive (MB/s, one decimal — DiskBandWidget's per-drive
// KB/s figures summed and rescaled), both rows sharing the same font
// (Fonts::loadValue). A letter ("R"/"W") is drawn after the value instead of
// NetworkTrafficWidget's direction arrow, at a position fixed relative to the
// widget's left edge — like that widget, the letter's position never depends
// on the rendered value string's measured width, only the digits grow/shrink
// to its left. Tapping anywhere on the widget opens the disk info screen
// (default SHOW_DISKS action, mirroring DiskBandWidget/FpsWidget).
class DiskSummaryWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    DiskSummaryWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                      PcMetrics& pcMetrics, EventType action = EventType::SHOW_DISKS,
                      ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;
    EventType action_;
    ActionCallback callback_;

    // Cached *rendered* text + colour (not the raw KB/s sum) so a value that
    // rounds to the same displayed text doesn't force a redraw — same
    // rationale as NetworkTrafficWidget's lastUpText_/lastDownText_.
    char lastReadText_[16] = "";
    char lastWriteText_[16] = "";
    uint16_t lastReadColor_ = 0;
    uint16_t lastWriteColor_ = 0;
    bool lastHasData_ = false;

    void drawRow(int16_t rowY, bool isRead, float mbps, bool hasData, bool forceRedraw,
                 char* lastText, size_t lastTextSize, uint16_t& lastColor);

    // Write-rate colour: a light-grey-to-light-green ramp (like
    // NetworkTrafficWidget::trafficColor()'s idle-to-moderate range) up to
    // 60% of kWriteCapMBps, then yellow/orange as it climbs further, only
    // reaching the (lightened) red at/over the cap — so anything well under
    // the cap never reads as red.
    uint16_t writeColor(float mbps);

    // KB/s -> MB/s.
    static constexpr float kKBpsToMBps = 1.0f / 1024.0f;

    // Write-rate colour scale cap, MB/s — red only appears at/over this rate.
    static constexpr float kWriteCapMBps = 150.0f;
};
