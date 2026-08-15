#pragma once

#include <Arduino.h>

#include <cstring>

#include "config/AppSettings.h"
#include "config/Limits.h"
#include "core/resources/FontRegistry.h"
#include "core/resources/weather_icons_44.h"
#include "services/weather/WeatherData.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"
#include "utils/ScopedLock.h"

#include <time.h>

// Full-width daily forecast column strip, covering the whole weather screen
// above the bottom back-button band.
//
// Layout (one column per day, 5..7 columns):
//
//   [ 44px icon ]          ← WMO weather-code icon, column-centred at top
//   MON                   ← abbreviated day name (value font, white)
//   06.08                 ← DD.MM date (label font, bright, tight below day)
//
//   24°                   ← max temp (18 pt, white)
//   14°                   ← min temp (18 pt, white)
//   mm 0.4                ← 'mm' label over the rain value
//   m/s 12.3              ← 'm/s' label over the wind value
//
// A little free space separates the date line from the first value row. The
// column count is driven by WeatherData::dayCount (7 for the configured API).
// Values use the smaller 15 pt value font so up to seven columns stay legible.
// Each cell is dirty-cached so only changed cells repaint.
//
// The widget ticks every second while displayed to detect a local-midnight
// rollover (compare device-local midnight against days[0].dayStart) and
// signals WeatherData::refreshRequested so WeatherJob refetches immediately,
// but throttles that cheap date check to weatherTimeCheckIntervalMs.
class WeatherWidget : public Widget {
 public:
    WeatherWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                  WeatherData& weatherData, const AppSettings& config);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    static constexpr uint8_t kMaxColumns = AppConfig::Limits::kForecastDays;
    static constexpr uint8_t kIconSize = AppConfig::Limits::kIconSize;

    // Vertical layout: icon; day name (larger) + date beneath it; then the temp
    // block (label + max + min) and the rain/wind rows, each running an even
    // blank gutter before its label.
    static constexpr int16_t kIconY = 6;
    static constexpr int16_t kDayNameY = 64;
    static constexpr int16_t kDateY = 78;
    static constexpr int16_t kTempMaxY = 128;
    static constexpr int16_t kTempMinY = 156;
    static constexpr int16_t kRainLabelY = 186;
    static constexpr int16_t kRainValueY = 204;
    static constexpr int16_t kWindLabelY = 224;
    static constexpr int16_t kWindValueY = 244;

    // Per-column cached snapshot for dirty detection. Equal-length arrays, one
    // element per possible column; only the first `columns_` are live.
    struct ColumnCache {
        int16_t iconWmo = -1;
        char dayName[4] = {0};
        char date[6] = {0};
        char tempMax[8] = {0};
        char tempMin[8] = {0};
        char rain[12] = {0};
        char wind[11] = {0};
    };

    WeatherData& weatherData_;
    const AppSettings& config_;

    // WeatherData is sticky-available (WeatherService never clears
    // availability on fetch failure) — freshness must be checked by age too,
    // or a stale outage-era forecast displays forever with no indication.
    // Timeout mirrors WeatherJob's own refetch threshold: once a refetch was
    // already due but hasn't landed, the display should say so.
    DataFreshnessGuard freshness_{weatherData_.freshness, config_.weatherRefreshIntervalMs};

    ColumnCache lastColumns_[kMaxColumns];
    uint8_t columns_ = 0;
    bool lastHasData_ = false;

    unsigned long lastTimeCheckMs_ = 0;

    // Column geometry for the current draw pass; recomputed on every count change.
    uint8_t colWidth_ = 0;
    int16_t leftPad_ = 0;

    void recomputeLayout(uint8_t count);
    int16_t columnCenter(uint8_t col) const;

    void checkMidnightRollover();

    const uint16_t* iconForWeatherCode(int16_t wmo) const;

    void drawIcon(int16_t wmo, uint8_t col);
    void drawDayHeader(const char* dayName, const char* date, uint16_t dayColor, uint8_t col);
    void drawTempBlock(const char* tempMax, const char* tempMin, uint8_t col);
    void drawLabeledValue(int16_t labelY, int16_t valueY, const char* label, const char* value,
                          uint16_t valueColor, uint8_t col);
    void drawNoData();
};