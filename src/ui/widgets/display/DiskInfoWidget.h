#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "config/LgfxConfig.h"
#include "services/pcMetrics/PcMetrics.h"
#include "ui/core/DisplayContext.h"
#include "ui/core/Layout.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/ScopedLock.h"

// Full-screen disk info list (the content area of the DISKS screen): one row
// per drive with a drive letter, a free-space usage bar + percent, and two
// right-hand columns for live read/write rates (always MB/s, rounded).
//
// Data snapshot logic mirrors PcMetricsWidget's disk tiles — everything is
// copied out of PcMetrics under ScopedLock, then all rendering happens
// lock-free. Rows are rebuilt only when the drive count changes; value-only
// redraws are gated on cached last-drawn state.
class DiskInfoWidget : public Widget {
 public:
    DiskInfoWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                   uint32_t updateIntervalMs, PcMetrics& pcMetrics);

    bool handleTouch(uint16_t x, uint16_t y) override;
    void setStaleTimeout(unsigned long timeoutMs) { freshnessGuard_.setTimeout(timeoutMs); }

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // -----------------------------------------------------------------------
    // Layout constants — all magic numbers live here
    // -----------------------------------------------------------------------
    static constexpr uint16_t kScreenWidth = Layout::kScreenW;

    // Maximum number of drives that fit in the 272 px content area.
    static constexpr uint16_t kMaxDisks = 10;
    static constexpr uint16_t kRowH = 27;

    // Row columns, left to right: drive letter, usage bar, percent, read rate,
    // write rate.
    static constexpr uint16_t kNameColW = 40;
    static constexpr uint16_t kBarX = 48;
    static constexpr uint16_t kBarRight = 238;
    static constexpr uint16_t kBarH = 14;
    // kReadX/kWriteX trimmed 8px closer to the percent column and to each
    // other (was 310/395) to widen both rate columns from 85px to 89px —
    // headroom for 4-figure NVMe rates (e.g. "3400") without clipping into
    // the "MB/s" label.
    static constexpr uint16_t kPercentRight = 298;  // NN% right-aligned here
    static constexpr uint16_t kReadX = 302;         // read rate column
    static constexpr uint16_t kWriteX = 391;        // write rate column

    // Rate column layout: a '|' column separator is pinned kRateSepX from the
    // column's left edge, then the (larger) 'R'/'W' activity letter at
    // kRateLetterX. The numeric value is right-aligned to the column's right
    // edge minus kRateValueRightGap (its "MB/s" suffix is also right-aligned,
    // with kRateValueLabelGap between them). All edges are fixed, so the
    // digits grow leftward instead of the text jittering left/right as the
    // value's digit count changes. kRateLetterX/kRateValueRightGap/
    // kRateValueLabelGap trimmed slightly (from 10/6/3) to claw back a few
    // more px of digit room alongside the column widening above.
    static constexpr uint16_t kRateSepX = 2;
    static constexpr uint16_t kRateLetterX = 8;
    static constexpr uint16_t kRateValueRightGap = 4;
    static constexpr uint16_t kRateValueLabelGap = 2;

    // Rate value + letter rendering: the value is always light grey (readable
    // on black) regardless of activity, but switches to white once the rounded
    // MB/s value exceeds kHighRateThresholdMbPerSec so hot transfers stand
    // out. The 'R'/'W' letter is the activity indicator — bright green (read)
    // / bright red kActivityWriteColor (write) on any non-zero activity, dark
    // grey (kIdleLetterColor) when idle. The stale color is
    // Colors::kInactiveText.
    static constexpr uint16_t kIdleRateColor = TFT_LIGHTGREY;
    static constexpr uint16_t kIdleLetterColor = TFT_DARKGREY;
    static constexpr uint16_t kActivityWriteColor = 0xF88C;  // light red (R31 G8 B12)
    static constexpr int kHighRateThresholdMbPerSec = 50;
    static constexpr uint16_t kHighRateColor = TFT_WHITE;

    // Unit suffixes ('MB/s', '%') never change, so they're drawn once per row
    // (on row creation) in a dim dark grey to keep them visually subordinate
    // to the changing values.
    static constexpr uint16_t kUnitColor = TFT_DARKGREY;
    static constexpr uint16_t kPercentValueGap = 3;

    // Rate column roles — index into DiskRow's cached rate state.
    static constexpr uint8_t kRateRead = 0;
    static constexpr uint8_t kRateWrite = 1;

    struct DiskRow {
        char name[4] = "";
        float freeSpacePercent = 0.0f;
        float readKBPerSec = 0.0f;
        float writeKBPerSec = 0.0f;

        // Last-drawn state — value-only redraw compares against these.
        bool drawn = false;
        int lastPercent = -1;
        uint16_t lastBarColor = 0xFFFF;  // sentinel: force first draw
        char lastRateText[2][20] = {{""}, {""}};
        uint16_t lastRateColor[2] = {0xFFFF, 0xFFFF};    // sentinel: force first draw
        uint16_t lastLetterColor[2] = {0xFFFF, 0xFFFF};  // sentinel: force first draw
    };

    // -----------------------------------------------------------------------
    // Dependencies
    // -----------------------------------------------------------------------
    DisplayContext& context_;
    PcMetrics& pcMetrics_;
    DataFreshnessGuard freshnessGuard_;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    bool lastAvailable_ = false;
    bool lastStale_ = false;
    std::vector<DiskRow> rows_;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void ensureRowsCreated();
    void drawRows(bool stale);
    void drawRow(size_t index, bool stale);
    void drawRate(size_t index, bool stale, uint8_t role);
    void drawStaticUnits(size_t index);
    void drawNoDataMessage();
    void clearArea();
    void formatRate(char* buf, size_t len, float kbPerSec) const;
};
