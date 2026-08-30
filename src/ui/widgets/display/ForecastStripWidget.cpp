#include "ForecastStripWidget.h"

#include <cstring>

#include "ui/core/UiText.h"
#include "ui/resources/FontRegistry.h"
#include "ui/widgets/display/WeatherFormat.h"
#include "utils/ScopedLock.h"

#include <time.h>

static constexpr uint16_t kDayColor = 0x8410;  // dim grey — same shade AirQualityWidget uses for unit labels
static constexpr uint16_t kTempMaxColor = TFT_WHITE;
static constexpr uint16_t kTempMinColor = TFT_LIGHTGREY;
static constexpr uint16_t kRainColor = kWeatherRainColor;
static constexpr uint16_t kNoRainColor = TFT_DARKGREY;

ForecastStripWidget::ForecastStripWidget(const WidgetInterface::Dimensions& dims,
                                         uint32_t updateIntervalMs, WeatherData& weatherData,
                                         const AppSettings& config, EventType action,
                                         ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      weatherData_(weatherData),
      config_(config),
      action_(action),
      callback_(std::move(callback)) {}

void ForecastStripWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    columns_ = 0;
    lastHasData_ = false;
    lastDataUpdateMsValid_ = false;
    for (auto& cache : lastColumns_) {
        cache = ColumnCache{};
    }
}

void ForecastStripWidget::onDraw(bool forceRedraw) {
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
    if (count > config_.multiWidgetForecastDays)
        count = config_.multiWidgetForecastDays;
    if (count > kMaxColumns)
        count = kMaxColumns;

    const bool layoutChanged = forceRedraw || !lastHasData_ || count != columns_;
    if (layoutChanged) {
        drawStatic();
        lastHasData_ = true;
        columns_ = count;
        recomputeLayout(count);
    }

    const unsigned long dataUpdateMs = weatherData_.freshness.lastUpdateMs();
    if (!layoutChanged && lastDataUpdateMsValid_ && dataUpdateMs == lastDataUpdateMs_) {
        // Forecast data hasn't changed since the last pass — nothing to
        // recompute (see P2-23: the field-diff checks below all evaluate
        // false anyway, so skip the mutex/localtime_r/snprintf work
        // entirely rather than paying it every 200ms tick).
        return;
    }
    lastDataUpdateMs_ = dataUpdateMs;
    lastDataUpdateMsValid_ = true;

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

        char tempMax[8];
        snprintf(tempMax, sizeof(tempMax), "%d\xc2\xb0", roundX10ToWhole(day.tempMaxX10));
        char tempMin[8];
        snprintf(tempMin, sizeof(tempMin), "%d\xc2\xb0", roundX10ToWhole(day.tempMinX10));

        char rain[8];
        if (day.rainX10 > 0)
            formatX10OneDecimal(rain, sizeof(rain), day.rainX10);
        else
            snprintf(rain, sizeof(rain), "-");

        const bool dayChanged = forceRedraw || strcmp(dayName, cache.dayName) != 0;
        const bool maxChanged = forceRedraw || strcmp(tempMax, cache.tempMax) != 0;
        const bool minChanged = forceRedraw || strcmp(tempMin, cache.tempMin) != 0;
        const bool rainChanged = forceRedraw || strcmp(rain, cache.rain) != 0;

        const uint16_t dayColor =
            (ltm.tm_wday == 0 || ltm.tm_wday == 6) ? kWeatherWeekendColor : kDayColor;
        const uint16_t rainColor = (day.rainX10 > 0) ? kRainColor : kNoRainColor;

        if (dayChanged || maxChanged || minChanged || rainChanged) {
            drawColumn(i, dayChanged, maxChanged, minChanged, rainChanged, dayName, dayColor,
                      tempMax, tempMin, rain, rainColor, i == 0);
        }

        strncpy(cache.dayName, dayName, sizeof(cache.dayName) - 1);
        cache.dayName[sizeof(cache.dayName) - 1] = '\0';
        strncpy(cache.tempMax, tempMax, sizeof(cache.tempMax) - 1);
        cache.tempMax[sizeof(cache.tempMax) - 1] = '\0';
        strncpy(cache.tempMin, tempMin, sizeof(cache.tempMin) - 1);
        cache.tempMin[sizeof(cache.tempMin) - 1] = '\0';
        strncpy(cache.rain, rain, sizeof(cache.rain) - 1);
        cache.rain[sizeof(cache.rain) - 1] = '\0';
    }
}

void ForecastStripWidget::recomputeLayout(uint8_t count) {
    colWidth_ = dimensions_.width / count;
    leftPad_ = static_cast<int16_t>((dimensions_.width - colWidth_ * count) / 2);

    // Column dividers, inset 2px top/bottom, between adjacent columns.
    LGFX* lcd = getLcd();
    for (uint8_t i = 1; i < count; ++i) {
        const int16_t x = dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * i;
        lcd->drawFastVLine(x, dimensions_.y + 2, dimensions_.height - 4, kDividerColor);
    }
}

int16_t ForecastStripWidget::columnCenter(uint8_t col) const {
    return dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col + colWidth_ / 2;
}

void ForecastStripWidget::checkMidnightRollover() {
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

    if (weatherData_.dayCount == 0) {
        return;
    }

    time_t firstDayStart;
    {
        ScopedLock lock(weatherData_.daysMutex);
        firstDayStart = weatherData_.days[0].dayStart;
    }

    if (localMidnight != firstDayStart) {
        weatherData_.refreshRequested.store(true);
    }
}

void ForecastStripWidget::drawColumn(uint8_t col, bool dayChanged, bool maxChanged,
                                     bool minChanged, bool rainChanged, const char* dayName,
                                     uint16_t dayColor, const char* tempMax, const char* tempMin,
                                     const char* rain, uint16_t rainColor, bool isToday) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);
    const int16_t colX = dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col;

    if (dayChanged) {
        lcd->fillRect(colX + 1, dimensions_.y, colWidth_ - 2, kMaxY - kDayY, TFT_BLACK);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(dayColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(dayName, cx, dimensions_.y + kDayY);
        Fonts::unload(lcd);
        if (isToday) {
            lcd->drawFastHLine(colX + colWidth_ / 4, dimensions_.y + kDayY + 9, colWidth_ / 2,
                               kTodayUnderlineColor);
        }
    }

    if (maxChanged) {
        lcd->fillRect(colX + 1, dimensions_.y + kMaxY - 12, colWidth_ - 2, 23, TFT_BLACK);
        Fonts::loadMetric(lcd);
        lcd->setTextColor(kTempMaxColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(tempMax, cx, dimensions_.y + kMaxY);
        Fonts::unload(lcd);
    }

    if (minChanged) {
        lcd->fillRect(colX + 1, dimensions_.y + kMinY - 10, colWidth_ - 2, 20, TFT_BLACK);
        Fonts::loadValue(lcd);
        lcd->setTextColor(kTempMinColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(tempMin, cx, dimensions_.y + kMinY);
        Fonts::unload(lcd);
    }

    if (rainChanged) {
        lcd->fillRect(colX + 1, dimensions_.y + kRainY - 9, colWidth_ - 2, 18, TFT_BLACK);
        Fonts::loadLabel(lcd);
        lcd->setTextColor(rainColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(rain, cx, dimensions_.y + kRainY);
        Fonts::unload(lcd);
    }
}

void ForecastStripWidget::drawNoData() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

bool ForecastStripWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    if (!callback_) {
        return false;
    }
    callback_(action_);
    return true;
}
