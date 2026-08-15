#include "DiskBandWidget.h"

#include <cstdio>
#include <cstring>

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

namespace {
// The shared diskActivityColor() scale treats anything below 2 MB/s as idle
// (dark grey kHairline). On the band we want idle lines to disappear into the
// background entirely, so treat rates up to 1 MB/s as idle and draw pure
// black, then clamp the base scale up to its lowest visible level (dark
// green) once it starts climbing.
uint16_t bandActivityColor(float kbPerSec) {
    constexpr float kIdleThresholdKbPerSec = 1.0f * 1024.0f;
    if (kbPerSec <= kIdleThresholdKbPerSec)
        return TFT_BLACK;
    const uint16_t color = Colors::diskActivityColor(kbPerSec);
    return (color == Colors::kHairline) ? TFT_DARKGREEN : color;
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
    diskWriteLineColor_.assign(snapshotCount, 0xFFFF);  // sentinel forces first draw
    diskReadLineColor_.assign(snapshotCount, 0xFFFF);
    diskFreeSpaceSmoothed_.assign(snapshotCount, -1.0f);  // sentinel: no previous value yet
    diskDriveNames_.clear();
    diskDriveNames_.reserve(snapshotCount);
    for (size_t i = 0; i < snapshotCount; ++i) {
        std::array<char, 4> name{};
        strncpy(name.data(), snapshot[i].name, name.size() - 1);
        diskDriveNames_.push_back(name);
    }

    const uint16_t maxWidgetWidth = kMaxWidgetWidth;
    const uint16_t availableWidth = (dimensions_.width > kChevronReservedWidth)
                                        ? (dimensions_.width - kChevronReservedWidth)
                                        : dimensions_.width;
    uint16_t widgetWidth = static_cast<uint16_t>(availableWidth / snapshotCount);
    if (widgetWidth > maxWidgetWidth)
        widgetWidth = maxWidgetWidth;

    // Tile fills the vertical span between the write line and the read line.
    const uint16_t diskAreaHeight = static_cast<uint16_t>(readLineYRelative() - kDiskAreaY);

    for (size_t i = 0; i < snapshotCount; ++i) {
        uint16_t xPos = static_cast<uint16_t>(dimensions_.x + i * widgetWidth);

        MetricWidget::Config config;
        config.value = snapshot[i].freeSpacePercent;
        config.unit = "";
        config.reverseThresholds = true;
        config.useDimColors = true;
        config.useSmallFont = true;
        config.label = snapshot[i].name;
        config.labelWidth = 14;   // narrow: the label is a single drive letter
        config.borderMargin = 0;  // flush against the activity lines — no edge gaps
        config.lowerThreshold = 0.0f;
        config.upperThreshold = 95.0f;

        auto w = std::make_unique<MetricWidget>(
            WidgetInterface::Dimensions{xPos, static_cast<uint16_t>(dimensions_.y + kDiskAreaY),
                                        widgetWidth, static_cast<uint16_t>(diskAreaHeight)},
            updateIntervalMs_, config);

        if (w) {
            if (!w->isInitialized())
                w->initialize(getContext());
            initAndDrawWidget(*w);
            diskDriveWidgets_.push_back(std::move(w));
        }
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
    // MetricWidget loading/unloading its own font per tick (see 07-performance.md
    // P1-5). No paired label-font pass is needed — disk tiles are built with
    // an empty unit (DiskBandWidget::ensureChildWidgetsCreated), so
    // drawUnitWithLoadedFont() would always be a no-op.
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
        const uint16_t writeColor = bandActivityColor(writeSnapshot[i]);
        if (i >= diskWriteLineColor_.size())
            continue;
        if (diskWriteLineColor_[i] != writeColor) {
            getLcd()->fillRect(dims.x, dimensions_.y + kWriteLineY, dims.width, kWriteLineHeight,
                               writeColor);
            diskWriteLineColor_[i] = writeColor;
        }

        const uint16_t readColor = bandActivityColor(readSnapshot[i]);
        if (diskReadLineColor_[i] != readColor) {
            getLcd()->fillRect(dims.x, dimensions_.y + readLineYRelative(), dims.width,
                               kReadLineHeight, readColor);
            diskReadLineColor_[i] = readColor;
        }
    }
}

void DiskBandWidget::drawFreshStatic() {
    ensureChildWidgetsCreated();
    for (auto& dw : diskDriveWidgets_) {
        if (dw)
            initAndDrawWidget(*dw);
    }
    drawDiskChevron();
}

void DiskBandWidget::drawDiskChevron() {
    if (diskDriveWidgets_.empty())
        return;
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(">", static_cast<int32_t>(dimensions_.x + dimensions_.width - 10),
                    (dimensions_.y + kWriteLineY + dimensions_.y + readLineYRelative()) / 2);
    Fonts::unload(lcd);
}

void DiskBandWidget::clearChildren() {
    // The widget's static content never overlaps its write line (draws at the
    // very top) or read line (draws at the very bottom). To make sure nothing
    // bleeds above/below the band, clear a background 2px taller and 1px up
    // from the widget's own bounds (widget size itself is unchanged).
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
    // width, and the band contains nothing else (write line .. read line).
    if (y >= dimensions_.y + kWriteLineY &&
        y < dimensions_.y + readLineYRelative() + kReadLineHeight) {
        callback_(action_);
        return true;
    }
    return false;
}
