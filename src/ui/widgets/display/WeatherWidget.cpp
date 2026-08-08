#include "WeatherWidget.h"

// ---------------------------------------------------------------------------
// Static styling
// ---------------------------------------------------------------------------

static constexpr uint16_t kDayColor = TFT_WHITE;
static constexpr uint16_t kDateColor = TFT_LIGHTGREY;
static constexpr uint16_t kLabelColor = TFT_DARKGREY;
static constexpr uint16_t kTempColor = TFT_WHITE;
static constexpr uint16_t kValueColor = TFT_LIGHTGREY;
static constexpr uint16_t kWeekendColor = 0xFBCF;  // light red for SAT/SUN day names
static constexpr uint16_t kRainColor = 0x867F;     // light blue, matches AirQuality humidity

// Abbreviated day names indexed by tm_wday (0 = Sunday).
static const char* const kDayNames[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// Converts a *10-scaled temperature to whole degrees, rounding to the nearest
// integer (half away from zero) instead of truncating — plain integer
// division on a negative X10 value truncates toward zero and displays up to
// 1° too warm.
static int16_t roundX10ToWhole(int16_t x10) {
    return static_cast<int16_t>(x10 >= 0 ? (x10 + 5) / 10 : (x10 - 5) / 10);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

WeatherWidget::WeatherWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             WeatherData& weatherData, const AppSettings& config)
    : Widget(dims, updateIntervalMs), weatherData_(weatherData), config_(config) {}

// ---------------------------------------------------------------------------
// drawStatic
// ---------------------------------------------------------------------------

void WeatherWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    columns_ = 0;
    lastHasData_ = false;
    // Reassign rather than memset — a raw zero-fill would clobber iconWmo's
    // -1 "no icon cached yet" sentinel with 0, which collides with WMO code 0
    // (clear sky). That collision then skips painting the icon on the first
    // real draw after data arrives, since iconChanged would compare 0 == 0.
    for (auto& cache : lastColumns_) {
        cache = ColumnCache{};
    }
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void WeatherWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    checkMidnightRollover();

    if (!freshness_.isFresh() || weatherData_.dayCount == 0) {
        if (forceRedraw || lastHasData_) {
            drawNoData();
            lastHasData_ = false;
            columns_ = 0;
        }
        return;
    }

    uint8_t count = weatherData_.dayCount;
    if (count > kMaxColumns) {
        count = kMaxColumns;
    }

    // Structural change (screen (re)entry, data first arriving, or the count
    // changing) — clear the whole area and treat every cell as changed.
    if (forceRedraw || !lastHasData_ || count != columns_) {
        drawStatic();
        lastHasData_ = true;
        columns_ = count;
        recomputeLayout(count);
    }

    for (uint8_t i = 0; i < count; ++i) {
        WeatherForecastDay day;
        {
            ScopedLock lock(weatherData_.daysMutex);
            day = weatherData_.days[i];
        }
        ColumnCache& cache = lastColumns_[i];

        struct tm ltm;
        localtime_r(&day.dayStart, &ltm);

        char dayName[4];
        snprintf(dayName, sizeof(dayName), "%s", kDayNames[ltm.tm_wday]);
        char date[6];
        snprintf(date, sizeof(date), "%02u.%02u", ltm.tm_mday, ltm.tm_mon + 1);

        char tempMax[8];
        snprintf(tempMax, sizeof(tempMax), "%d\xc2\xb0", roundX10ToWhole(day.tempMaxX10));
        char tempMin[8];
        snprintf(tempMin, sizeof(tempMin), "%d\xc2\xb0", roundX10ToWhole(day.tempMinX10));

        char rain[8];
        snprintf(rain, sizeof(rain), "%d.%d", day.rainX10 / 10, day.rainX10 % 10);
        char wind[8];
        snprintf(wind, sizeof(wind), "%d.%d", day.windMaxX10 / 10, day.windMaxX10 % 10);

        const bool iconChanged = forceRedraw || day.weatherCode != cache.iconWmo;
        const bool headerChanged =
            forceRedraw || strcmp(dayName, cache.dayName) != 0 || strcmp(date, cache.date) != 0;
        const bool tempChanged = forceRedraw || strcmp(tempMax, cache.tempMax) != 0 ||
                                 strcmp(tempMin, cache.tempMin) != 0;
        const bool rainChanged = forceRedraw || strcmp(rain, cache.rain) != 0;
        const bool windChanged = forceRedraw || strcmp(wind, cache.wind) != 0;

        // SAT/SUN in light red, other days white.
        const uint16_t dayColor =
            (ltm.tm_wday == 0 || ltm.tm_wday == 6) ? kWeekendColor : kDayColor;
        // Any precipitation tints the value in light blue.
        const uint16_t rainColor = (day.rainX10 > 0) ? kRainColor : kValueColor;

        if (iconChanged)
            drawIcon(day.weatherCode, i);
        if (headerChanged)
            drawDayHeader(dayName, date, dayColor, i);
        if (tempChanged)
            drawTempBlock(tempMax, tempMin, i);
        if (rainChanged)
            drawLabeledValue(kRainLabelY, kRainValueY, "mm", rain, rainColor, i);
        if (windChanged)
            drawLabeledValue(kWindLabelY, kWindValueY, "m/s", wind, kValueColor, i);

        cache.iconWmo = day.weatherCode;
        strncpy(cache.dayName, dayName, sizeof(cache.dayName) - 1);
        cache.dayName[sizeof(cache.dayName) - 1] = '\0';
        strncpy(cache.date, date, sizeof(cache.date) - 1);
        cache.date[sizeof(cache.date) - 1] = '\0';
        strncpy(cache.tempMax, tempMax, sizeof(cache.tempMax) - 1);
        cache.tempMax[sizeof(cache.tempMax) - 1] = '\0';
        strncpy(cache.tempMin, tempMin, sizeof(cache.tempMin) - 1);
        cache.tempMin[sizeof(cache.tempMin) - 1] = '\0';
        strncpy(cache.rain, rain, sizeof(cache.rain) - 1);
        cache.rain[sizeof(cache.rain) - 1] = '\0';
        strncpy(cache.wind, wind, sizeof(cache.wind) - 1);
        cache.wind[sizeof(cache.wind) - 1] = '\0';
    }
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

void WeatherWidget::recomputeLayout(uint8_t count) {
    colWidth_ = dimensions_.width / count;
    leftPad_ = static_cast<int16_t>((dimensions_.width - colWidth_ * count) / 2);
}

int16_t WeatherWidget::columnCenter(uint8_t col) const {
    return dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col + colWidth_ / 2;
}

// ---------------------------------------------------------------------------
// Midnight rollover detection
// ---------------------------------------------------------------------------

void WeatherWidget::checkMidnightRollover() {
    const unsigned long now = millis();
    if (now - lastTimeCheckMs_ < config_.weatherTimeCheckIntervalMs) {
        return;
    }
    lastTimeCheckMs_ = now;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return;
    }
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    const time_t localMidnight = mktime(&timeinfo);

    time_t firstDayStart;
    {
        ScopedLock lock(weatherData_.daysMutex);
        firstDayStart = weatherData_.days[0].dayStart;
    }

    if (weatherData_.dayCount > 0 && localMidnight != firstDayStart) {
        weatherData_.refreshRequested.store(true);
    }
}

// ---------------------------------------------------------------------------
// WMO → icon mapping (day icons only; a daily forecast has no day/night split)
// ---------------------------------------------------------------------------

const uint16_t* WeatherWidget::iconForWeatherCode(int16_t wmo) const {
    if (wmo == 0 || wmo == 1)
        return icon_01d;  // clear sky
    if (wmo == 2)
        return icon_02d;  // partly cloudy
    if (wmo == 3)
        return icon_04d;  // overcast
    if (wmo == 45 || wmo == 48)
        return icon_50d;  // fog / rime fog
    if (wmo >= 51 && wmo <= 57)
        return icon_09d;  // drizzle
    if (wmo >= 80 && wmo <= 82)
        return icon_09d;  // rain showers
    if (wmo >= 71 && wmo <= 77)
        return icon_13d;  // snow
    if (wmo >= 95)
        return icon_11d;  // thunderstorm / hail
    if (wmo >= 85 && wmo <= 86)
        return icon_13d;  // snow showers
    if (wmo == 66 || wmo == 67)
        return icon_10d;  // freezing rain
    if (wmo >= 61 && wmo <= 65)
        return icon_10d;  // rain
    return nullptr;
}

// ---------------------------------------------------------------------------
// Cell renderers
// ---------------------------------------------------------------------------

void WeatherWidget::drawDayHeader(const char* dayName, const char* date, uint16_t dayColor,
                                  uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);

    // Day name is larger than the date beneath it, so draw each on its own font.
    Fonts::loadValue(lcd);
    lcd->setTextColor(dayColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(dayName, cx, dimensions_.y + kDayNameY);
    Fonts::unload(lcd);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(kDateColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(date, cx, dimensions_.y + kDateY);
    Fonts::unload(lcd);
}

void WeatherWidget::drawIcon(int16_t wmo, uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);

    lcd->fillRect(dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col,
                  dimensions_.y + kIconY, colWidth_, kIconSize, TFT_BLACK);

    const uint16_t* data = iconForWeatherCode(wmo);
    if (data) {
        lcd->pushImage(cx - kIconSize / 2, dimensions_.y + kIconY, kIconSize, kIconSize, data);
    }
}

void WeatherWidget::drawTempBlock(const char* tempMax, const char* tempMin, uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);

    // Bright max value.
    Fonts::loadMetric(lcd);
    lcd->setTextColor(kTempColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(tempMax, cx, dimensions_.y + kTempMaxY);
    Fonts::unload(lcd);

    // Min value beneath, same colour and size as the max value.
    Fonts::loadMetric(lcd);
    lcd->setTextColor(kTempColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(tempMin, cx, dimensions_.y + kTempMinY);
    Fonts::unload(lcd);
}

void WeatherWidget::drawLabeledValue(int16_t labelY, int16_t valueY, const char* label,
                                     const char* value, uint16_t valueColor, uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);

    // Label above the value.
    Fonts::loadLabel(lcd);
    lcd->setTextColor(kLabelColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(label, cx, dimensions_.y + labelY);
    Fonts::unload(lcd);

    // Value below.
    Fonts::loadMetric(lcd);
    lcd->setTextColor(valueColor, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(value, cx, dimensions_.y + valueY);
    Fonts::unload(lcd);
}

void WeatherWidget::drawNoData() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("NO DATA", dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}