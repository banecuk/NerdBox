#pragma once

#include <time.h>

#include "ui/widgets/base/Widget.h"
#include "utils/CalendarMath.h"

// Month-grid calendar for CalendarScreen. Owns the displayed month cursor
// (year_/month_), defaulting to today's date, and repaints only when the
// cursor month changes, today's day-of-month rolls over, or on forceRedraw.
//
// Layout (full 480×272 content area above the bottom band):
//
//   y=0..34    month title, e.g. "AUGUST 2026" — confined to x=[44,436) so
//              the screen's prev/next-month arrow buttons (44px each side,
//              drawn on top by CalendarScreen) never get overpainted.
//   y=34..58   weekday header row, "MON TUE WED THU FRI SAT SUN", full width
//   y=58..272  6 rows × 7 columns of day numbers, full width
//
// Today's cell gets a filled accent circle behind the number. Leading/
// trailing padding days (outside the displayed month) are left blank.
class CalendarWidget : public Widget {
 public:
    explicit CalendarWidget(const WidgetInterface::Dimensions& dims,
                            uint32_t updateIntervalMs = 1000);

    // Moves the displayed cursor by `delta` months (called by the screen's
    // arrow buttons).
    void stepMonth(int8_t delta);

    // Resets the cursor to the current month (today).
    void resetToToday();

    bool handleTouch(uint16_t x, uint16_t y) override { return false; }

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint8_t kCols = 7;
    static constexpr uint8_t kRows = 6;
    static constexpr uint16_t kTitleH = 34;
    static constexpr uint16_t kWeekdayH = 24;
    static constexpr uint16_t kGridY = kTitleH + kWeekdayH;

    // Title text is confined between the screen's arrow buttons so a
    // title-only repaint never overpaints them.
    static constexpr uint16_t kTitleInset = 44;

    static constexpr uint16_t kTodayAccent = 0x051D;  // dim blue accent

    int year_ = 1970;
    uint8_t month_ = 1;

    // Cached state used to decide whether a repaint is needed.
    int lastDrawnYear_ = 0;
    uint8_t lastDrawnMonth_ = 0;
    int8_t lastDrawnToday_ = -1;
    bool everDrawn_ = false;

    uint16_t colWidth_ = 0;
    uint16_t rowHeight_ = 0;
    int16_t leftPad_ = 0;

    void drawWeekdayHeader();
    void drawTitle();
    void drawGrid(int8_t todayMday);
    void drawNoData();

    int16_t cellCenterX(uint8_t col) const;
    int16_t cellCenterY(uint8_t row) const;
};
