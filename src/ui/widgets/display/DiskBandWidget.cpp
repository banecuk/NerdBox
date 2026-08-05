#include "DiskBandWidget.h"

#include <cstdio>
#include <cstring>

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

namespace {
// The shared diskActivityColor() scale treats anything below 2 MB/s as idle
// (dark grey kHairline), which is indistinguishable from the background on the
// band's thin activity lines. On the band we only want to light up when there
// is real traffic, so treat rates up to 1 MB/s as idle and clamp the base
// scale up to its lowest visible level (dark green) once it starts climbing.
uint16_t bandActivityColor(float kbPerSec) {
    constexpr float kIdleThresholdKbPerSec = 1.0f * 1024.0f;
    if (kbPerSec <= kIdleThresholdKbPerSec)
        return Colors::kHairline;
    const uint16_t color = Colors::diskActivityColor(kbPerSec);
    return (color == Colors::kHairline) ? TFT_DARKGREEN : color;
}
}  // namespace

DiskBandWidget::DiskBandWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                               uint32_t updateIntervalMs, PcMetrics& pcMetrics,
                               EventType action, ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      pcMetrics_(pcMetrics),
      action_(action),
      callback_(std::move(callback)),
      freshnessGuard_(pcMetrics.is_available, pcMetrics.last_update_timestamp) {}

void DiskBandWidget::ensureDiskWidgetsCreated() {
    // Snapshot everything we need under one lock, then do all widget work lock-free.
    struct DriveSnapshot {
        char name[4];
        int freeSpacePercent;
    };
    std::vector<DriveSnapshot> snapshot;
    {
        PcMetricsDiskLock lock(pcMetrics_);
        const size_t driveCount = pcMetrics_.disk_drives.size() < kMaxDiskWidgets
                                      ? pcMetrics_.disk_drives.size()
                                      : kMaxDiskWidgets;
        snapshot.reserve(driveCount);
        for (size_t i = 0; i < driveCount; ++i) {
            DriveSnapshot s;
            strncpy(s.name, pcMetrics_.disk_drives[i].driveName, sizeof(s.name) - 1);
            s.name[sizeof(s.name) - 1] = '\0';
            s.freeSpacePercent =
                static_cast<int>(pcMetrics_.disk_drives[i].freeSpacePercent + 0.5f);
            snapshot.push_back(s);
        }
    }  // mutex released here — all remaining work is lock-free

    if (snapshot.empty())
        return;

    const bool needsCreation =
        diskDriveWidgets_.empty() || (diskDriveWidgets_.size() != snapshot.size());

    if (!needsCreation)
        return;

    // Rebuild widgets from the snapshot
    diskDriveWidgets_.clear();
    diskWriteLineColor_.assign(snapshot.size(), 0xFFFF);  // sentinel forces first draw
    diskReadLineColor_.assign(snapshot.size(), 0xFFFF);
    diskFreeSpaceSmoothed_.assign(snapshot.size(), -1.0f);  // sentinel: no previous value yet

    const uint16_t maxWidgetWidth = kMaxWidgetWidth;
    uint16_t widgetWidth = static_cast<uint16_t>(dimensions_.width / snapshot.size());
    if (widgetWidth > maxWidgetWidth)
        widgetWidth = maxWidgetWidth;

    // Tile fills the vertical span between the write line and the read line.
    const uint16_t diskAreaHeight = static_cast<uint16_t>(readLineYRelative() - kDiskAreaY);

    for (size_t i = 0; i < snapshot.size(); ++i) {
        uint16_t xPos = static_cast<uint16_t>(dimensions_.x + i * widgetWidth);

        auto w = MetricWidget::Builder(
                     WidgetInterface::Dimensions{
                         xPos, static_cast<uint16_t>(dimensions_.y + kDiskAreaY), widgetWidth,
                         static_cast<uint16_t>(diskAreaHeight)},
                     updateIntervalMs_)
                     .unit("")
                     .range(0, 100)
                     .colorThresholds(0.0f, 95.0f)
                     .reverseThresholds(true)
                     .useDimColors(true)
                     .smallFont()
                     .label(snapshot[i].name)
                     .labelWidth(14)  // narrow: the label is a single drive letter
                     .value(snapshot[i].freeSpacePercent)
                     .borderMargin(0)  // flush against the activity lines — no edge gaps
                     .build();

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

void DiskBandWidget::updateDiskDriveWidgets() {
    // Snapshot the free-space values under the lock, then release it before
    // touching the display.  Holding PcMetricsDiskLock across draw() would
    // keep the mutex taken while rendering pixels — a priority inversion risk
    // that can block the background fetch task and cause missed frames.
    // ensureDiskWidgetsCreated() uses the same snapshot pattern.
    const size_t widgetCount = diskDriveWidgets_.size();
    if (widgetCount == 0)
        return;

    // One float per widget slot (raw, pre-smoothing).
    float freeSpaceRawSnapshot[kMaxDiskWidgets];
    float writeSnapshot[kMaxDiskWidgets];
    float readSnapshot[kMaxDiskWidgets];
    size_t updateCount = 0;
    {
        PcMetricsDiskLock lock(pcMetrics_);
        updateCount = (pcMetrics_.disk_drives.size() < widgetCount) ? pcMetrics_.disk_drives.size()
                                                                    : widgetCount;
        for (size_t i = 0; i < updateCount; ++i) {
            freeSpaceRawSnapshot[i] = pcMetrics_.disk_drives[i].freeSpacePercent;
            writeSnapshot[i] = pcMetrics_.disk_drives[i].writeKBPerSec;
            readSnapshot[i] = pcMetrics_.disk_drives[i].readKBPerSec;
        }
    }  // mutex released — all display work below is lock-free

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
        diskDriveWidgets_[i]->draw(false);

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
            getLcd()->fillRect(dims.x, dimensions_.y + readLineYRelative(), dims.width, kReadLineHeight,
                               readColor);
            diskReadLineColor_[i] = readColor;
        }
    }
}

void DiskBandWidget::initAndDrawWidget(MetricWidget& widget) {
    // initialize() draws static chrome on first call only (no-op on repeat
    // calls once the widget is already initialized), so the explicit
    // drawStatic() below is what actually repaints chrome on every
    // subsequent stale->fresh transition. forceRefresh() resets the cached
    // layout state and performs the initial value draw immediately.
    widget.initialize(getContext());
    widget.drawStatic();
    widget.forceRefresh();
}

void DiskBandWidget::onDrawStatic() {
    if (hasFreshData()) {
        ensureDiskWidgetsCreated();
        for (auto& dw : diskDriveWidgets_) {
            if (dw)
                initAndDrawWidget(*dw);
        }
        drawDiskChevron();
    } else {
        clearDiskWidgets();
    }
}

void DiskBandWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool currentlyHasFreshData = hasFreshData();
    const bool stateChanged = (wasFreshData_ != currentlyHasFreshData);

    if (stateChanged) {
        if (!currentlyHasFreshData) {
            clearDiskWidgets();
        } else {
            clearDiskWidgets();
            drawStatic();
            drawDiskChevron();
            clearDirty();
        }
        wasFreshData_ = currentlyHasFreshData;
    }

    if (currentlyHasFreshData && pcMetrics_.last_update_timestamp != lastEnsureCheckTimestamp_) {
        ensureDiskWidgetsCreated();
        lastEnsureCheckTimestamp_ = pcMetrics_.last_update_timestamp;
    }

    const bool needsRedraw = forceRedraw || isDirty() || needsUpdate();
    if (currentlyHasFreshData && needsRedraw) {
        updateDiskDriveWidgets();
        drawDiskChevron();
        clearDirty();
    }

    lastUpdateTimeMs_ = millis();
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

void DiskBandWidget::clearDiskWidgets() {
    // The widget's static content never overlaps its write line (draws at the
    // very top) or read line (draws at the very bottom). To make sure nothing
    // bleeds above/below the band, clear a background 2px taller and 1px up
    // from the widget's own bounds (widget size itself is unchanged).
    getLcd()->fillRect(dimensions_.x, dimensions_.y - 1, dimensions_.width,
                       dimensions_.height + 2, TFT_BLACK);

    diskDriveWidgets_.clear();
    diskWriteLineColor_.clear();
    diskReadLineColor_.clear();
    diskFreeSpaceSmoothed_.clear();

    lastEnsureCheckTimestamp_ = 0;
    isStaticDrawn_ = false;
}

bool DiskBandWidget::needsUpdate() const {
    if (!isInitialized_)
        return false;
    if (hasFreshData() != wasFreshData_)
        return true;
    return (pcMetrics_.last_update_timestamp > lastUpdateTimestamp_) ||
           (millis() - lastUpdateTimeMs_ >= updateIntervalMs_);
}

bool DiskBandWidget::handleTouch(uint16_t x, uint16_t y) {
    if (!callback_ || diskDriveWidgets_.empty())
        return false;

    // The whole strip is tappable: the drive tiles span the widget's full
    // width, and the band contains nothing else (write line .. read line).
    if (y >= dimensions_.y + kWriteLineY && y < dimensions_.y + readLineYRelative() + kReadLineHeight) {
        callback_(action_);
        return true;
    }
    return false;
}
