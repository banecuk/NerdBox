#include "DiskBandWidget.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"
#include "ui/resources/FontRegistry.h"

namespace {
// The shared Colors::disk*ActivityColor() scales treat anything below 1 MB/s
// as idle (dark grey kHairline). On the band we want idle lines to disappear
// into the background entirely, so treat rates at/below that same threshold
// as idle and draw pure black instead. The kHairline fallback below is
// defensive only — Colors won't actually return it once the two thresholds
// match, but it protects against the scales' idle threshold ever changing
// independently again.
uint16_t bandActivityColor(float kbPerSec, bool isWrite) {
    constexpr float kIdleThresholdKbPerSec = 1.0f * 1024.0f;
    if (kbPerSec <= kIdleThresholdKbPerSec)
        return TFT_BLACK;
    const uint16_t color = isWrite ? Colors::diskWriteActivityColor(kbPerSec)
                                    : Colors::diskReadActivityColor(kbPerSec);
    if (color != Colors::kHairline)
        return color;
    // TFT_DARKRED is a buggy LovyanGFX constant (duplicate of TFT_DARKMAGENTA's
    // value) that decodes to olive/yellow-green rather than red — see
    // Colors::diskWriteActivityColor(). TFT_MAROON is the correct dark red.
    return isWrite ? TFT_MAROON : TFT_DARKGREEN;
}
}  // namespace

DiskBandWidget::DiskBandWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                               uint32_t updateIntervalMs, PcMetrics& pcMetrics, EventType action,
                               ActionCallback callback)
    : PcDataCompositeWidget(dims, updateIntervalMs, pcMetrics),
      action_(action),
      callback_(std::move(callback)) {}

void DiskBandWidget::ensureChildWidgetsCreated() {
    // Snapshot everything we need under one lock, then do all widget work
    // lock-free. Fixed-size array, not a vector: this runs every time fresh
    // data lands (every ~500 ms), and in the overwhelmingly common case
    // (drive set unchanged) the snapshot is discarded a few lines below —
    // a heap alloc/free pair on that cadence would fragment the
    // fragmentation-sensitive heap for no reason. kMaxDiskWidgets is already
    // a compile-time cap.
    struct DriveSnapshot {
        char name[4];
        int freeSpacePercent;
    };
    DriveSnapshot snapshot[kMaxDiskWidgets];
    size_t snapshotCount = 0;
    {
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        snapshotCount = pcMetrics_.disk_drives.size() < kMaxDiskWidgets
                            ? pcMetrics_.disk_drives.size()
                            : kMaxDiskWidgets;
        for (size_t i = 0; i < snapshotCount; ++i) {
            DriveSnapshot& s = snapshot[i];
            strncpy(s.name, pcMetrics_.disk_drives[i].driveName, sizeof(s.name) - 1);
            s.name[sizeof(s.name) - 1] = '\0';
            s.freeSpacePercent =
                static_cast<int>(pcMetrics_.disk_drives[i].freeSpacePercent + 0.5f);
        }
    }  // mutex released here — all remaining work is lock-free

    if (snapshotCount == 0)
        return;

    bool needsCreation = diskDriveWidgets_.empty() || (diskDriveWidgets_.size() != snapshotCount);
    if (!needsCreation) {
        for (size_t i = 0; i < snapshotCount; ++i) {
            if (strncmp(diskDriveNames_[i].data(), snapshot[i].name, sizeof(snapshot[i].name)) !=
                0) {
                needsCreation = true;
                break;
            }
        }
    }

    if (!needsCreation)
        return;

    // Rebuild widgets from the snapshot
    diskDriveWidgets_.clear();
    diskDriveNames_.clear();
    diskWriteLineColor_.assign(snapshotCount, 0xFFFF);  // sentinel forces first draw
    diskReadLineColor_.assign(snapshotCount, 0xFFFF);
    diskFreeSpaceSmoothed_.assign(snapshotCount, -1.0f);  // sentinel: no previous value yet
    diskDriveWidgets_.reserve(snapshotCount);
    diskDriveNames_.reserve(snapshotCount);

    const uint16_t maxWidgetWidth = kMaxWidgetWidth;
    const uint16_t availableWidth = dimensions_.width;
    uint16_t widgetWidth = static_cast<uint16_t>(availableWidth / snapshotCount);
    const bool uncapped = widgetWidth <= maxWidgetWidth;
    if (widgetWidth > maxWidgetWidth)
        widgetWidth = maxWidgetWidth;

    // Tile fills the vertical span below the activity line and its gap.
    const uint16_t diskAreaHeight = static_cast<uint16_t>(dimensions_.height - kDiskAreaY);

    for (size_t i = 0; i < snapshotCount; ++i) {
        uint16_t xPos = static_cast<uint16_t>(dimensions_.x + i * widgetWidth);
        // Last tile absorbs any leftover pixels from integer division so the
        // band's right edge is flush with the widget bounds — but only when
        // tiles aren't already capped narrower than an even split, else this
        // would stretch the last tile far wider than its siblings.
        uint16_t tileWidth = widgetWidth;
        if (uncapped && i == snapshotCount - 1)
            tileWidth = static_cast<uint16_t>(dimensions_.x + dimensions_.width - xPos);

        MetricWidget::Config config;
        config.value = snapshot[i].freeSpacePercent;
        config.unit = "";
        config.reverseThresholds = true;
        config.useDimColors = true;
        config.useSmallFont = true;
        config.label = snapshot[i].name;
        config.labelWidth = kLabelWidth;  // narrow: the label is a single drive letter
        config.borderMargin = 0;  // flush against the activity lines — no edge gaps
        config.lowerThreshold = 0.0f;
        config.upperThreshold = 95.0f;
        config.gradientBackground = true;

        auto w = std::make_unique<MetricWidget>(
            WidgetInterface::Dimensions{xPos, static_cast<uint16_t>(dimensions_.y + kDiskAreaY),
                                        tileWidth, static_cast<uint16_t>(diskAreaHeight)},
            updateIntervalMs_, config);

        // diskDriveNames_ and diskDriveWidgets_ are pushed together here, in
        // the same loop iteration, so their sizes can never drift apart —
        // see B23: they used to be filled by two separate loops, relying on
        // the second loop's `if (w)` guard never actually skipping an entry.
        if (!w)
            continue;

        if (!w->isInitialized())
            w->initialize(getContext());
        initAndDrawWidget(*w);

        std::array<char, 4> name{};
        strncpy(name.data(), snapshot[i].name, name.size() - 1);
        diskDriveNames_.push_back(name);
        diskDriveWidgets_.push_back(std::move(w));
    }

    if (getLogger()) {
        getLogger()->debugf("Created %d disk drive widgets", diskDriveWidgets_.size());
    }
}

void DiskBandWidget::drawDynamicData() {
    // Snapshot the free-space values under the lock, then release it before
    // touching the display.  Holding ScopedLock across draw() would
    // keep the mutex taken while rendering pixels — a priority inversion risk
    // that can block the background fetch task and cause missed frames.
    // ensureChildWidgetsCreated() uses the same snapshot pattern.
    const size_t widgetCount = diskDriveWidgets_.size();
    if (widgetCount == 0)
        return;

    // One float per widget slot (raw, pre-smoothing).
    float freeSpaceRawSnapshot[kMaxDiskWidgets];
    float writeSnapshot[kMaxDiskWidgets];
    float readSnapshot[kMaxDiskWidgets];
    size_t updateCount = 0;
    {
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        updateCount = (pcMetrics_.disk_drives.size() < widgetCount) ? pcMetrics_.disk_drives.size()
                                                                    : widgetCount;
        for (size_t i = 0; i < updateCount; ++i) {
            freeSpaceRawSnapshot[i] = pcMetrics_.disk_drives[i].freeSpacePercent;
            writeSnapshot[i] = pcMetrics_.disk_drives[i].writeKBPerSec;
            readSnapshot[i] = pcMetrics_.disk_drives[i].readKBPerSec;
        }
    }  // mutex released — all display work below is lock-free

    // Batch font load: every tile here is useSmallFont_ (NotoSansDisplay15),
    // so one loadValue()/unload() pair covers all drives instead of each
    // MetricWidget calling setFont() on its own per tick. No paired
    // label-font pass is needed — disk tiles are built with an empty unit
    // (DiskBandWidget::ensureChildWidgetsCreated), so drawUnitWithLoadedFont()
    // would always be a no-op.
    LGFX* lcd = getLcd();
    Fonts::loadValue(lcd);
    for (size_t i = 0; i < updateCount; ++i) {
        if (!diskDriveWidgets_[i] || !diskDriveWidgets_[i]->isInitialized())
            continue;

        // Smooth falling free-space values: if the new reading is lower than
        // the previously displayed value, show the average of the two instead
        // of jumping straight down, and carry that average forward as the new
        // previous value. Rising values (or the first sample) are shown as-is.
        const float raw = freeSpaceRawSnapshot[i];
        const float previous =
            (i < diskFreeSpaceSmoothed_.size()) ? diskFreeSpaceSmoothed_[i] : -1.0f;
        const float smoothed = (previous >= 0.0f && raw < previous) ? (previous + raw) / 2.0f : raw;
        if (i < diskFreeSpaceSmoothed_.size())
            diskFreeSpaceSmoothed_[i] = smoothed;
        const int freeSpaceValue = static_cast<int>(smoothed + 0.5f);

        if (diskDriveWidgets_[i]->getValue() != freeSpaceValue) {
            diskDriveWidgets_[i]->setValue(freeSpaceValue);
        }
        diskDriveWidgets_[i]->drawValueWithLoadedFont();
    }
    Fonts::unload(lcd);

    for (size_t i = 0; i < updateCount; ++i) {
        if (!diskDriveWidgets_[i] || !diskDriveWidgets_[i]->isInitialized())
            continue;

        const auto dims = diskDriveWidgets_[i]->getDimensions();
        if (i >= diskWriteLineColor_.size())
            continue;

        // Activity line is split left/right within the tile to mirror the
        // MetricWidget label/value split below it: the read half spans the
        // drive-letter label column, the write half fills the rest (the
        // larger value-display area).
        const uint16_t readWidth = static_cast<uint16_t>(
            dims.width < kLabelColumnWidth ? dims.width : kLabelColumnWidth);
        const uint16_t writeWidth = static_cast<uint16_t>(dims.width - readWidth);
        const uint16_t lineY = dimensions_.y;

        const uint16_t readColor = bandActivityColor(readSnapshot[i], /*isWrite=*/false);
        if (diskReadLineColor_[i] != readColor) {
            getLcd()->fillRect(dims.x, lineY, readWidth, kActivityLineHeight, readColor);
            diskReadLineColor_[i] = readColor;
        }

        const uint16_t writeColor = bandActivityColor(writeSnapshot[i], /*isWrite=*/true);
        if (diskWriteLineColor_[i] != writeColor) {
            getLcd()->fillRect(static_cast<int32_t>(dims.x + readWidth), lineY, writeWidth,
                               kActivityLineHeight, writeColor);
            diskWriteLineColor_[i] = writeColor;
        }
    }
}

void DiskBandWidget::drawFreshStatic() {
    ensureChildWidgetsCreated();
    for (auto& dw : diskDriveWidgets_) {
        if (dw)
            initAndDrawWidget(*dw);
    }
}

void DiskBandWidget::clearChildren() {
    // The widget's static content never overlaps its activity line (draws at
    // the very top). To make sure nothing bleeds above/below the band, clear
    // a background 2px taller and 1px up from the widget's own bounds
    // (widget size itself is unchanged).
    getLcd()->fillRect(dimensions_.x, dimensions_.y - 1, dimensions_.width, dimensions_.height + 2,
                       TFT_BLACK);

    diskDriveWidgets_.clear();
    diskWriteLineColor_.clear();
    diskReadLineColor_.clear();
    diskFreeSpaceSmoothed_.clear();
    diskDriveNames_.clear();
}

bool DiskBandWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!callback_ || diskDriveWidgets_.empty())
        return false;

    // The whole strip is tappable: the drive tiles span the widget's full
    // width and height.
    if (y >= dimensions_.y && y < dimensions_.y + dimensions_.height) {
        callback_(action_);
        return true;
    }
    return false;
}
