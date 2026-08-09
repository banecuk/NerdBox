#include "DiskInfoWidget.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "core/resources/FontRegistry.h"
#include "ui/core/Colors.h"

DiskInfoWidget::DiskInfoWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                               uint32_t updateIntervalMs, PcMetrics& pcMetrics)
    : Widget(dims, updateIntervalMs),
      context_(context),
      pcMetrics_(pcMetrics),
      freshnessGuard_(pcMetrics.freshness) {}

void DiskInfoWidget::onDrawStatic() {
    clearArea();
}

void DiskInfoWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool available = pcMetrics_.freshness.available();
    const bool stale = available && !freshnessGuard_.isFresh();

    // Full repaint on state transitions: availability flipped, staleness
    // flipped, or a forced redraw (chrome dirty / screen enter).
    if (forceRedraw || available != lastAvailable_ || stale != lastStale_) {
        lastAvailable_ = available;
        lastStale_ = stale;
        rows_.clear();
        clearArea();
    }

    if (!available) {
        drawNoDataMessage();
        lastUpdateTimeMs_ = millis();
        clearDirty();
        return;
    }

    ensureRowsCreated();
    drawRows(stale);

    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void DiskInfoWidget::ensureRowsCreated() {
    struct DriveSnapshot {
        char name[4];
        float freeSpacePercent;
        float readKBPerSec;
        float writeKBPerSec;
    };

    // Snapshot everything under one lock, then do all display work lock-free.
    std::vector<DriveSnapshot> snapshot;
    {
        ScopedLock lock(pcMetrics_.disk_drivesMutex);
        const size_t driveCount =
            pcMetrics_.disk_drives.size() < kMaxDisks ? pcMetrics_.disk_drives.size() : kMaxDisks;
        snapshot.reserve(driveCount);
        for (size_t i = 0; i < driveCount; ++i) {
            DriveSnapshot s;
            strncpy(s.name, pcMetrics_.disk_drives[i].driveName, sizeof(s.name) - 1);
            s.name[sizeof(s.name) - 1] = '\0';
            s.freeSpacePercent = pcMetrics_.disk_drives[i].freeSpacePercent;
            s.readKBPerSec = pcMetrics_.disk_drives[i].readKBPerSec;
            s.writeKBPerSec = pcMetrics_.disk_drives[i].writeKBPerSec;
            snapshot.push_back(s);
        }
    }  // mutex released here — all remaining work is lock-free

    if (snapshot.empty()) {
        rows_.clear();
        return;
    }

    // Rebuild rows when the drive count changed, or the count stayed the same
    // but the set of drives itself did (e.g. D: unplugged, E: appears at the
    // same index) — a count-only comparison would leave the old row's cached
    // name/state on screen forever.
    bool needsCreation = rows_.size() != snapshot.size();
    if (!needsCreation) {
        for (size_t i = 0; i < snapshot.size(); ++i) {
            if (strncmp(rows_[i].name, snapshot[i].name, sizeof(snapshot[i].name)) != 0) {
                needsCreation = true;
                break;
            }
        }
    }
    if (needsCreation) {
        rows_.clear();
        rows_.reserve(snapshot.size());
        for (const auto& s : snapshot) {
            DiskRow r;
            strncpy(r.name, s.name, sizeof(r.name) - 1);
            r.name[sizeof(r.name) - 1] = '\0';
            rows_.push_back(r);
        }
        clearArea();  // clean slate before drawing the new row set
    }

    for (size_t i = 0; i < snapshot.size(); ++i) {
        rows_[i].freeSpacePercent = snapshot[i].freeSpacePercent;
        rows_[i].readKBPerSec = snapshot[i].readKBPerSec;
        rows_[i].writeKBPerSec = snapshot[i].writeKBPerSec;
    }
}

void DiskInfoWidget::drawRows(bool stale) {
    for (size_t i = 0; i < rows_.size(); ++i) {
        drawRow(i, stale);
    }
}

void DiskInfoWidget::drawRow(size_t index, bool stale) {
    DiskRow& r = rows_[index];
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const uint16_t rowY = dimensions_.y + static_cast<uint16_t>(index * kRowH);
    const uint16_t centerY = rowY + kRowH / 2;
    const uint16_t barY = rowY + (kRowH - kBarH) / 2;

    // Clamp for the bar width and the color lookup (both use 0..100).
    const int percent = static_cast<int>(r.freeSpacePercent + 0.5f);
    const int percentClamped = percent > 100 ? 100 : (percent < 0 ? 0 : percent);

    // Bar fill color from the free-space percent. The gradient runs
    // low=blue/green → high=red, so pass the inverted value to match the main
    // screen's disk tiles (reversed thresholds on free space): a nearly-full
    // disk reads red, an almost-empty disk reads green.
    const uint16_t barColor =
        stale
            ? Colors::kInactiveText
            : context_.getColors().getColorFromPercent(static_cast<uint8_t>(100 - percentClamped));
    // Bar width tracks occupied space (100 - free%), not free space, so a
    // nearly-full disk shows a long red bar rather than a short one.
    const uint16_t barW = static_cast<uint16_t>(
        (static_cast<uint32_t>(kBarRight - kBarX) * (100 - percentClamped)) / 100);

    // Usage bar — track + fill, redrawn only when the value or color changed.
    if (!r.drawn || r.lastPercent != percentClamped || r.lastBarColor != barColor) {
        lcd->fillRect(dimensions_.x + kBarX, barY, kBarRight - kBarX, kBarH, TFT_BLACK);
        lcd->fillRect(dimensions_.x + kBarX, barY, kBarRight - kBarX, kBarH, Colors::kHairline);
        if (barW > 0) {
            lcd->fillRect(dimensions_.x + kBarX, barY, barW, kBarH, barColor);
        }
        r.lastPercent = percentClamped;
        r.lastBarColor = barColor;
    }

    // Drive letter — header font, centered in the left column, aligned with
    // the row's vertical center.
    if (!r.drawn) {
        Fonts::loadHeader(lcd);
        lcd->setTextColor(stale ? Colors::kInactiveText : TFT_LIGHTGREY, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        const int16_t nameW = lcd->textWidth(r.name);
        lcd->drawString(r.name, dimensions_.x + static_cast<int16_t>((kNameColW - nameW) / 2),
                        centerY);
        Fonts::unload(lcd);

        // Static '%' and "MB/s" units — drawn exactly once per row, before any
        // value redraw below clears its own (non-overlapping) region.
        drawStaticUnits(index);
    }

    // Free-space percent — metric font, right-aligned at kPercentRight. The
    // '%' unit sits at the fixed right edge in kUnitColor (drawn once by
    // drawStaticUnits) while the value grows leftward of it; only the value
    // area is cleared here so the unit survives.
    if (!r.drawn || r.lastPercent != percentClamped) {
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", percentClamped);
        Fonts::loadMetric(lcd);
        lcd->setTextDatum(MC_DATUM);
        const int16_t unitW = lcd->textWidth("%");

        lcd->fillRect(dimensions_.x + kBarRight, rowY,
                      kPercentRight - kBarRight - static_cast<uint16_t>(unitW), kRowH, TFT_BLACK);

        lcd->setTextColor(stale ? Colors::kInactiveText : TFT_LIGHTGREY, TFT_BLACK);
        const int16_t numW = lcd->textWidth(numBuf);
        lcd->drawString(numBuf,
                        dimensions_.x + static_cast<int16_t>(kPercentRight - unitW -
                                                             kPercentValueGap - numW / 2),
                        centerY);
        Fonts::unload(lcd);
    }

    // Read/write rate columns — label font, each colored by its own activity.
    drawRate(index, stale, kRateRead);
    drawRate(index, stale, kRateWrite);

    r.drawn = true;
}

void DiskInfoWidget::drawRate(size_t index, bool stale, uint8_t role) {
    DiskRow& r = rows_[index];
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const uint16_t rowY = dimensions_.y + static_cast<uint16_t>(index * kRowH);
    const uint16_t centerY = rowY + kRowH / 2;

    const float kb = (role == kRateRead) ? r.readKBPerSec : r.writeKBPerSec;
    const uint16_t colX = (role == kRateRead) ? kReadX : kWriteX;
    const uint16_t valueRightX =
        (role == kRateRead) ? (kWriteX - kRateValueRightGap) : (kScreenWidth - kRateValueRightGap);

    char rateText[16];
    formatRate(rateText, sizeof(rateText), kb);

    const bool hasActivity = kb > 0.0f;
    const int rateMb = static_cast<int>(kb / 1024.0f + 0.5f);
    const uint16_t valueColor =
        stale ? Colors::kInactiveText
              : (rateMb > kHighRateThresholdMbPerSec ? kHighRateColor : kIdleRateColor);
    const uint16_t letterColor =
        stale ? Colors::kInactiveText
              : (hasActivity ? ((role == kRateRead) ? TFT_GREEN : kActivityWriteColor)
                             : kIdleLetterColor);

    if (!r.drawn || strcmp(rateText, r.lastRateText[role]) != 0 ||
        r.lastRateColor[role] != valueColor || r.lastLetterColor[role] != letterColor) {
        // The static "MB/s" label (drawn once by drawStaticUnits) sits
        // right-aligned at valueRightX, and the static '|' separator at
        // kRateSepX; only the letter + value region is cleared here so both
        // survive value redraws. labelW must be measured with the label font
        // loaded, matching drawStaticUnits.
        Fonts::loadLabel(lcd);
        const int16_t labelW = lcd->textWidth("MB/s");

        const uint16_t clearX = colX + kRateLetterX;
        lcd->fillRect(dimensions_.x + clearX, rowY,
                      valueRightX - clearX - static_cast<uint16_t>(labelW), kRowH, TFT_BLACK);
        Fonts::unload(lcd);

        // Activity letter + rate value — larger value font, letter pinned just
        // right of the '|', value right-aligned to the "MB/s" label.
        Fonts::loadValue(lcd);
        lcd->setTextColor(letterColor, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString((role == kRateRead) ? "R" : "W",
                        dimensions_.x + static_cast<int16_t>(clearX), centerY);

        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        const int16_t valueW = lcd->textWidth(rateText);
        lcd->drawString(rateText,
                        dimensions_.x + static_cast<int16_t>(valueRightX - labelW -
                                                             kRateValueLabelGap - valueW / 2),
                        centerY);
        Fonts::unload(lcd);

        strncpy(r.lastRateText[role], rateText, sizeof(r.lastRateText[role]) - 1);
        r.lastRateText[role][sizeof(r.lastRateText[role]) - 1] = '\0';
        r.lastRateColor[role] = valueColor;
        r.lastLetterColor[role] = letterColor;
    }
}

// Static '%' and "MB/s" units plus the '|' column separators — drawn exactly
// once per row (on row creation), since they never change. The value redraws
// in drawRow/drawRate are scoped to clear only the dynamic regions so these
// survive until the row is rebuilt.
void DiskInfoWidget::drawStaticUnits(size_t index) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const uint16_t rowY = dimensions_.y + static_cast<uint16_t>(index * kRowH);
    const uint16_t centerY = rowY + kRowH / 2;

    // '%' — metric font, right-aligned to kPercentRight.
    Fonts::loadMetric(lcd);
    lcd->setTextColor(kUnitColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    const int16_t percentUnitW = lcd->textWidth("%");
    lcd->drawString("%", dimensions_.x + static_cast<int16_t>(kPercentRight - percentUnitW / 2),
                    centerY);
    Fonts::unload(lcd);

    // "MB/s" — label font, right-aligned to each rate column's valueRightX.
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kUnitColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    const int16_t labelW = lcd->textWidth("MB/s");
    const uint16_t readRightX = kWriteX - kRateValueRightGap;
    const uint16_t writeRightX = kScreenWidth - kRateValueRightGap;
    lcd->drawString("MB/s", dimensions_.x + static_cast<int16_t>(readRightX - labelW / 2), centerY);
    lcd->drawString("MB/s", dimensions_.x + static_cast<int16_t>(writeRightX - labelW / 2),
                    centerY);
    Fonts::unload(lcd);

    // '|' column separators — label font, dark grey, pinned just left of each
    // activity letter. Static, drawn once with the row.
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kUnitColor, TFT_BLACK);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString("|", dimensions_.x + static_cast<int16_t>(kReadX + kRateSepX), centerY);
    lcd->drawString("|", dimensions_.x + static_cast<int16_t>(kWriteX + kRateSepX), centerY);
    Fonts::unload(lcd);
}

void DiskInfoWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;
    Fonts::loadMetric(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("No Data", dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

void DiskInfoWidget::clearArea() {
    if (!getLcd())
        return;
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       TFT_BLACK);
}

void DiskInfoWidget::formatRate(char* buf, size_t len, float kbPerSec) const {
    snprintf(buf, len, "%d", static_cast<int>(kbPerSec / 1024.0f + 0.5f));
}

bool DiskInfoWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
