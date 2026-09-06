#include "ProcessListWidget.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"
#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

namespace {
constexpr uint16_t kBgColor = TFT_BLACK;
constexpr uint16_t kHeaderColor = TFT_DARKGREY;
constexpr uint16_t kValueColor = TFT_WHITE;
constexpr uint16_t kSeparatorColor = Colors::kBorderGrey;
}  // namespace

ProcessListWidget::ProcessListWidget(DisplayContext& context, const WidgetInterface::Dimensions& dims,
                                     uint32_t updateIntervalMs, const ProcessData& data)
    : Widget(dims, updateIntervalMs), data_(data), freshnessGuard_(data.freshness) {
    columnWidth_ = dimensions_.width / kColumns;
    const uint16_t gridHeight = dimensions_.height - kHeaderHeight;
    rowHeight_ = gridHeight / kRows;
}

void ProcessListWidget::onDrawStatic() {
    getLcd()->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                       kBgColor);
    for (auto& column : lastRow_) {
        for (auto& row : column) {
            row = RowText{};
        }
    }
    drawHeaders();
}

void ProcessListWidget::drawHeaders() {
    static const char* const labels[kColumns] = {"CPU", "RAM", "DISK"};
    LGFX* lcd = getLcd();
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kHeaderColor, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    for (uint8_t c = 0; c < kColumns; ++c) {
        const uint16_t x = dimensions_.x + c * columnWidth_ + columnWidth_ / 2;
        lcd->drawString(labels[c], x, dimensions_.y + kHeaderHeight / 2);
        if (c > 0) {
            const uint16_t sepX = dimensions_.x + c * columnWidth_;
            lcd->drawFastVLine(sepX, dimensions_.y, dimensions_.height, kSeparatorColor);
        }
    }
    Fonts::unload(lcd);
}

void ProcessListWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    const bool fresh = freshnessGuard_.isFresh();

    if (!wasFresh_ && fresh) {
        onDrawStatic();
    }

    if (fresh) {
        drawColumn(kCpu, data_.byCpu, data_.byCpuCount, forceRedraw || !wasFresh_);
        drawColumn(kRam, data_.byRam, data_.byRamCount, forceRedraw || !wasFresh_);
        drawColumn(kDisk, data_.byDisk, data_.byDiskCount, forceRedraw || !wasFresh_);
    } else if (wasFresh_ || forceRedraw) {
        drawNoDataMessage();
    }

    wasFresh_ = fresh;
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

void ProcessListWidget::formatEntry(Column column, const ProcessEntry& entry, RowText& out) {
    snprintf(out.name, sizeof(out.name), "%s", entry.name);
    switch (column) {
        case kCpu:
            if (entry.cpuPercent < 0.0f) {
                snprintf(out.value, sizeof(out.value), "-");
            } else {
                snprintf(out.value, sizeof(out.value), "%.0f%%", entry.cpuPercent);
            }
            break;
        case kRam:
            if (entry.ramMB > 1024.0f) {
                snprintf(out.value, sizeof(out.value), "%.1fG", entry.ramMB / 1024.0f);
            } else {
                snprintf(out.value, sizeof(out.value), "%.0f", entry.ramMB);
            }
            break;
        case kDisk:
            if (entry.diskKBPerSec > 1024.0f) {
                snprintf(out.value, sizeof(out.value), "%.1fM", entry.diskKBPerSec / 1024.0f);
            } else {
                snprintf(out.value, sizeof(out.value), "%.0f", entry.diskKBPerSec);
            }
            break;
    }
}

void ProcessListWidget::drawColumn(Column column, const ProcessEntry* entries, uint8_t count,
                                   bool forceRedraw) {
    for (uint8_t r = 0; r < kRows; ++r) {
        RowText text;
        uint8_t percentColor = 0;
        bool hasColor = false;

        if (r < count) {
            formatEntry(column, entries[r], text);
            if (column == kCpu && entries[r].cpuPercent >= 0.0f) {
                const float clamped =
                    entries[r].cpuPercent > 100.0f ? 100.0f : entries[r].cpuPercent;
                percentColor = static_cast<uint8_t>(clamped * 0.99f);
                hasColor = true;
            }
        }

        RowText& last = lastRow_[column][r];
        if (!forceRedraw && strcmp(text.name, last.name) == 0 &&
            strcmp(text.value, last.value) == 0) {
            continue;
        }
        last = text;
        drawRow(column, r, text, percentColor, hasColor);
    }
}

void ProcessListWidget::drawRow(Column column, uint8_t row, const RowText& text,
                                uint8_t percentColor, bool hasColor) {
    LGFX* lcd = getLcd();
    const uint16_t x = dimensions_.x + column * columnWidth_;
    const uint16_t y = dimensions_.y + kHeaderHeight + row * rowHeight_;
    lcd->fillRect(x, y, columnWidth_, rowHeight_, kBgColor);

    if (text.name[0] == '\0') {
        return;
    }

    Fonts::loadLabel(lcd);
    lcd->setTextColor(kValueColor, kBgColor);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(text.name, x + 4, y + rowHeight_ / 2);

    lcd->setTextColor(
        hasColor ? getContext().getColors().getColorFromPercentGrayGreen(percentColor)
                : kValueColor,
        kBgColor);
    lcd->setTextDatum(MR_DATUM);
    lcd->drawString(text.value, x + columnWidth_ - 4, y + rowHeight_ / 2);
    Fonts::unload(lcd);
}

void ProcessListWidget::drawNoDataMessage() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, kBgColor);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, kBgColor);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}
