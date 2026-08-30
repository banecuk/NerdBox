#pragma once

#include <cstdint>
#include <functional>

#include "config/AppSettings.h"
#include "config/Limits.h"
#include "core/events/EventTypes.h"
#include "services/weather/WeatherData.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Compact daily-forecast strip — MultiWidget's default (resting-state)
// candidate. Not a squeeze of WeatherWidget: that one is a full-screen
// 480x272 widget with 44px icons and a 9-row layout; nothing about it
// survives 82px, so this is its own layout sharing only WeatherFormat.h with
// it. Shows day name / max temp / min temp / rain per column, no icon —
// weather severity is conveyed by the rain value's colour instead.
//
// Layout (480 x 104, one column per day, config_.multiWidgetForecastDays of
// them — 6 by default, 80px each — up to kMaxColumns):
//
//   MON   TUE   WED   THU   FRI     <- day name (weekend in red), today underlined
//   24°   26°   22°   19°   21°     <- max temp, white
//   14°   15°   13°   11°   12°     <- min temp, light grey
//   0.4    -    2.1   8.6    -      <- rain mm, blue if > 0 else a dash
//
// See docs-local/09-multiwidget-rotation-and-forecast-strip.md.
class ForecastStripWidget : public Widget {
 public:
    using ActionCallback = std::function<void(EventType)>;

    ForecastStripWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                        WeatherData& weatherData, const AppSettings& config,
                        EventType action = EventType::NONE, ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint8_t kMaxColumns = AppConfig::Limits::kForecastDays;

    // Row gaps sized for the fonts, with extra breathing room between the
    // day name and the max-temp row below it; flow re-centered in
    // MultiWidget's current 103px height.
    static constexpr int16_t kDayY = 18;   // MC_DATUM centre for the 12pt day name
    static constexpr int16_t kMaxY = 43;  // 18pt max temp
    static constexpr int16_t kMinY = 64;  // 15pt min temp
    static constexpr int16_t kRainY = 85;  // 12pt rain

    static constexpr uint16_t kDividerColor = 0x18C3;   // same very-dark-grey row-border color
    static constexpr uint16_t kTodayUnderlineColor = 0x2965;

    // Per-column cached snapshot for dirty detection.
    struct ColumnCache {
        char dayName[4] = {0};
        char tempMax[8] = {0};
        char tempMin[8] = {0};
        char rain[8] = {0};
    };

    WeatherData& weatherData_;
    const AppSettings& config_;

    // WeatherData is sticky-available — freshness must be checked by age too,
    // or a stale outage-era forecast displays forever with no indication.
    DataFreshnessGuard freshness_{weatherData_.freshness, config_.weatherRefreshIntervalMs};

    EventType action_;
    ActionCallback callback_;

    ColumnCache lastColumns_[kMaxColumns];
    uint8_t columns_ = 0;
    bool lastHasData_ = false;

    unsigned long lastTimeCheckMs_ = 0;

    uint16_t colWidth_ = 0;
    int16_t leftPad_ = 0;

    void recomputeLayout(uint8_t count);
    int16_t columnCenter(uint8_t col) const;

    void checkMidnightRollover();

    void drawColumn(uint8_t col, bool dayChanged, bool maxChanged, bool minChanged,
                    bool rainChanged, const char* dayName, uint16_t dayColor, const char* tempMax,
                    const char* tempMin, const char* rain, uint16_t rainColor, bool isToday);
    void drawNoData();
};
