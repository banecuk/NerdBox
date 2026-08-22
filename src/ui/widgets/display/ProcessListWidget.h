#pragma once

#include "services/processes/ProcessData.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Content widget for ProcessesScreen: three side-by-side columns (CPU / RAM /
// Disk), each a header plus ProcessData::kEntriesPerList rows. Reads a const
// ProcessData&, gated on freshness.available() the same way CpuClockWidget
// checks DataFreshnessGuard — this data only arrives while the screen itself
// is active (see ProcessStreamJob's screen gate), so "stale" here just means
// "just opened the screen, first event hasn't landed yet."
class ProcessListWidget : public Widget {
 public:
    ProcessListWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                      uint32_t updateIntervalMs, const ProcessData& data);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint8_t kColumns = 3;
    static constexpr uint16_t kHeaderHeight = 24;
    static constexpr uint8_t kRows = ProcessData::kEntriesPerList;

    enum Column : uint8_t { kCpu = 0, kRam = 1, kDisk = 2 };

    const ProcessData& data_;
    DataFreshnessGuard freshnessGuard_;

    uint16_t columnWidth_ = 0;
    uint16_t rowHeight_ = 0;
    bool wasFresh_ = false;

    // Cached last-rendered text per cell, so onDraw only repaints rows whose
    // text actually changed — same lesson as AudioWidget's progress bar.
    char lastRow_[kColumns][kRows][32] = {};

    void drawHeaders();
    void drawColumn(Column column, const ProcessEntry* entries, uint8_t count, bool forceRedraw);
    void drawRow(Column column, uint8_t row, const char* text, uint8_t percentColor,
                bool hasColor);
    void formatEntry(Column column, const ProcessEntry& entry, char* out, size_t outLen);
    void drawNoDataMessage();
};
