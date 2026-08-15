#include "CalendarWidget.h"

#include <cstdio>

#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"

namespace {
// Abbreviated Monday-first weekday names for the header row.
constexpr const char* kWeekdayNames[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

// Monday-first column indices of Saturday/Sunday.
constexpr uint8_t kSatCol = 5;
constexpr uint8_t kSunCol = 6;

bool isWeekendCol(uint8_t col) {
    return col == kSatCol || col == kSunCol;
}

// Same light-red tint WeatherWidget uses for SAT/SUN day names, so the two
// screens agree on how weekends are called out.
constexpr uint16_t kWeekendColor = 0xFBCF;
}  // namespace

CalendarWidget::CalendarWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
    : Widget(dims, updateIntervalMs) {
    colWidth_ = dimensions_.width / kCols;
    leftPad_ = static_cast<int16_t>((dimensions_.width - colWidth_ * kCols) / 2);
    rowHeight_ = (dimensions_.height - kGridY) / kRows;
}

void CalendarWidget::stepMonth(int8_t delta) {
    CalendarMath::stepMonth(year_, month_, delta);
    markDirty();
}

void CalendarWidget::resetToToday() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5))
        return;
    year_ = timeinfo.tm_year + 1900;
    month_ = static_cast<uint8_t>(timeinfo.tm_mon + 1);
    markDirty();
}

// ---------------------------------------------------------------------------
// drawStatic
// ---------------------------------------------------------------------------

void CalendarWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    drawWeekdayHeader();

    lastDrawnYear_ = 0;
    lastDrawnMonth_ = 0;
    lastDrawnToday_ = -1;
    everDrawn_ = false;
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void CalendarWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    struct tm timeinfo;
    const bool timeOk = getLocalTime(&timeinfo, 5);

    if (!timeOk) {
        if (forceRedraw || everDrawn_) {
            drawNoData();
            everDrawn_ = false;
        }
        return;
    }

    // First successful time read since (re)entry — seed the cursor to
    // today's month rather than the 1970-01 default.
    if (!everDrawn_) {
        year_ = timeinfo.tm_year + 1900;
        month_ = static_cast<uint8_t>(timeinfo.tm_mon + 1);
    }

    const bool isCurrentMonth =
        (year_ == timeinfo.tm_year + 1900) && (month_ == timeinfo.tm_mon + 1);
    const int8_t today = isCurrentMonth ? static_cast<int8_t>(timeinfo.tm_mday) : int8_t(-1);

    const bool monthChanged =
        forceRedraw || !everDrawn_ || year_ != lastDrawnYear_ || month_ != lastDrawnMonth_;
    const bool todayChanged = today != lastDrawnToday_;

    if (!monthChanged && !todayChanged)
        return;

    if (monthChanged)
        drawTitle();
    drawGrid(today);

    lastDrawnYear_ = year_;
    lastDrawnMonth_ = month_;
    lastDrawnToday_ = today;
    everDrawn_ = true;
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

int16_t CalendarWidget::cellCenterX(uint8_t col) const {
    return dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col + colWidth_ / 2;
}

int16_t CalendarWidget::cellCenterY(uint8_t row) const {
    return dimensions_.y + kGridY + static_cast<int16_t>(rowHeight_) * row + rowHeight_ / 2;
}

// ---------------------------------------------------------------------------
// Renderers
// ---------------------------------------------------------------------------

void CalendarWidget::drawWeekdayHeader() {
    LGFX* lcd = getLcd();

    Fonts::loadLabel(lcd);
    lcd->setTextDatum(MC_DATUM);
    for (uint8_t col = 0; col < kCols; ++col) {
        lcd->setTextColor(isWeekendCol(col) ? kWeekendColor : TFT_DARKGREY, TFT_BLACK);
        lcd->drawString(kWeekdayNames[col], cellCenterX(col),
                        dimensions_.y + kTitleH + kWeekdayH / 2);
    }
    Fonts::unload(lcd);
}

void CalendarWidget::drawTitle() {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x + kTitleInset, dimensions_.y, dimensions_.width - 2 * kTitleInset,
                  kTitleH, TFT_BLACK);

    char title[24];
    snprintf(title, sizeof(title), "%s %d", CalendarMath::monthName(month_), year_);

    Fonts::loadHeader(lcd);
    lcd->setTextColor(TFT_WHITE, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(title, dimensions_.x + dimensions_.width / 2, dimensions_.y + kTitleH / 2);
    Fonts::unload(lcd);
}

void CalendarWidget::drawGrid(int8_t todayMday) {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y + kGridY, dimensions_.width,
                  dimensions_.height - kGridY, TFT_BLACK);

    const uint8_t firstWeekday = CalendarMath::firstWeekdayMonFirst(year_, month_);
    const uint8_t daysCount = CalendarMath::daysInMonth(year_, month_);

    int prevYear = year_;
    uint8_t prevMonth = month_;
    CalendarMath::stepMonth(prevYear, prevMonth, -1);
    const uint8_t prevDaysCount = CalendarMath::daysInMonth(prevYear, prevMonth);

    Fonts::loadValue(lcd);
    lcd->setTextDatum(MC_DATUM);

    // Leading padding — tail end of the previous month, dim grey.
    for (uint8_t col = 0; col < firstWeekday; ++col) {
        const uint8_t day = static_cast<uint8_t>(prevDaysCount - firstWeekday + 1 + col);
        char label[3];
        snprintf(label, sizeof(label), "%d", day);
        lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
        lcd->drawString(label, cellCenterX(col), cellCenterY(0));
    }

    for (uint8_t day = 1; day <= daysCount; ++day) {
        const uint16_t cellIndex = firstWeekday + day - 1;
        const uint8_t row = static_cast<uint8_t>(cellIndex / kCols);
        if (row >= kRows)
            break;
        const uint8_t col = static_cast<uint8_t>(cellIndex % kCols);

        const int16_t cx = cellCenterX(col);
        const int16_t cy = cellCenterY(row);

        const bool isToday = (day == todayMday);
        if (isToday) {
            const uint8_t radius = static_cast<uint8_t>(min(colWidth_, rowHeight_) / 2 - 2);
            lcd->fillCircle(cx, cy, radius, kTodayAccent);
        }

        char label[3];
        snprintf(label, sizeof(label), "%d", day);
        lcd->setTextColor(isToday ? TFT_BLACK : TFT_WHITE, isToday ? kTodayAccent : TFT_BLACK);
        lcd->drawString(label, cx, cy);
    }

    // Trailing padding — start of the next month, dim grey. Fills every
    // remaining cell through the end of the 6×7 grid.
    const uint16_t lastCellIndex = firstWeekday + daysCount - 1;
    const uint16_t gridCellCount = static_cast<uint16_t>(kRows * kCols);
    uint8_t nextDay = 1;
    for (uint16_t cellIndex = lastCellIndex + 1; cellIndex < gridCellCount; ++cellIndex) {
        const uint8_t row = static_cast<uint8_t>(cellIndex / kCols);
        const uint8_t col = static_cast<uint8_t>(cellIndex % kCols);
        char label[3];
        snprintf(label, sizeof(label), "%d", nextDay++);
        lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
        lcd->drawString(label, cellCenterX(col), cellCenterY(row));
    }

    Fonts::unload(lcd);
}

void CalendarWidget::drawNoData() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y + kGridY, dimensions_.width,
                  dimensions_.height - kGridY, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + kGridY + (dimensions_.height - kGridY) / 2);
    Fonts::unload(lcd);
}
