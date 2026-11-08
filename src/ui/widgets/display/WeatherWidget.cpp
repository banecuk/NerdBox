#include "WeatherWidget.h"

#include "ui/core/UiText.h"
#include "ui/widgets/base/WidgetPainter.h"
#include "ui/widgets/display/WeatherFormat.h"

// ---------------------------------------------------------------------------
// Static styling
// ---------------------------------------------------------------------------

static constexpr uint16_t kDayColor = TFT_WHITE;
static constexpr uint16_t kDateColor = TFT_LIGHTGREY;
static constexpr uint16_t kLabelColor = TFT_DARKGREY;
static constexpr uint16_t kTempColor = TFT_WHITE;
static constexpr uint16_t kValueColor = TFT_LIGHTGREY;
static constexpr uint16_t kWeekendColor = kWeatherWeekendColor;
static constexpr uint16_t kRainColor = kWeatherRainColor;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

WeatherWidget::WeatherWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                             const WeatherData& weatherData, const AppSettings& config)
    : Widget(dims, updateIntervalMs), weatherData_(weatherData), config_(config) {}

// ---------------------------------------------------------------------------
// drawStatic
// ---------------------------------------------------------------------------

void WeatherWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    columns_ = 0;
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

    const auto edge = freshness_.poll(weatherData_.dayCount > 0);
    if (!freshness_.fresh()) {
        if (forceRedraw || edge == FreshnessTransition::Edge::BecameStale) {
            drawNoData();
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
    if (forceRedraw || edge == FreshnessTransition::Edge::BecameFresh || count != columns_) {
        drawStatic();
        columns_ = count;
        recomputeLayout(count);
        drawColumnDividers();
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

        char rain[12];
        formatX10OneDecimal(rain, sizeof(rain), day.rainX10);
        char wind[12];
        formatX10OneDecimal(wind, sizeof(wind), day.windMaxX10);

        const bool iconChanged = forceRedraw || day.weatherCode != cache.iconWmo;
        const bool headerChanged =
            forceRedraw || !cache.dayName.equals(dayName) || !cache.date.equals(date);
        const bool tempChanged =
            forceRedraw || !cache.tempMax.equals(tempMax) || !cache.tempMin.equals(tempMin);
        const bool rainChanged = forceRedraw || !cache.rain.equals(rain);
        const bool windChanged = forceRedraw || !cache.wind.equals(wind);

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
            drawLabeledValue(kRainBandY, kWindBandY, kRainLabelY, kRainValueY, "mm", rain,
                             rainColor, i);
        if (windChanged)
            drawLabeledValue(kWindBandY, static_cast<int16_t>(dimensions_.height), kWindLabelY,
                             kWindValueY, "m/s", wind, kValueColor, i);

        cache.iconWmo = day.weatherCode;
        cache.dayName.set(dayName);
        cache.date.set(date);
        cache.tempMax.set(tempMax);
        cache.tempMin.set(tempMin);
        cache.rain.set(rain);
        cache.wind.set(wind);
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

// Gives the 7-column strip some structure for near-zero cost (see
// docs-local/03-visual-ux.md V13) — a hairline that fades in from black at
// the top and back out to black at the bottom, rather than a hard line
// spanning the full height.
void WeatherWidget::drawColumnDividers() const {
    if (columns_ < 2)
        return;

    LGFX* lcd = getLcd();
    const int16_t topHalf = dimensions_.height / 2;
    const int16_t midY = dimensions_.y + topHalf;

    constexpr uint16_t kBlack = TFT_BLACK;
    for (uint8_t col = 1; col < columns_; ++col) {
        const int16_t x = dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col;
        lcd->drawGradientVLine(x, dimensions_.y, topHalf, kBlack, Colors::kHairline);
        lcd->drawGradientVLine(x, midY, dimensions_.height - topHalf, Colors::kHairline, kBlack);
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

// Clears one column's [bandTop, bandBottom) slice (widget-relative y). Starts
// 1 px right of the column's left edge, where drawColumnDividers() draws, so
// the divider survives.
void WeatherWidget::clearCellBand(uint8_t col, int16_t bandTop, int16_t bandBottom) {
    const int16_t colLeft = dimensions_.x + leftPad_ + static_cast<int16_t>(colWidth_) * col;
    getLcd()->fillRect(colLeft + 1, dimensions_.y + bandTop, colWidth_ - 1, bandBottom - bandTop,
                       TFT_BLACK);
}

void WeatherWidget::drawDayHeader(const char* dayName, const char* date, uint16_t dayColor,
                                  uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);
    clearCellBand(col, kHeaderBandY, kTempBandY);

    // Day name is larger than the date beneath it, so draw each on its own font.
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + kDayNameY, dayName, Fonts::loadValue,
                                    dayColor);
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + kDateY, date, Fonts::loadLabel,
                                    kDateColor);
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
    clearCellBand(col, kTempBandY, kRainBandY);

    // Bright max value.
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + kTempMaxY, tempMax, Fonts::loadMetric,
                                    kTempColor);

    // Min value beneath, same colour and size as the max value.
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + kTempMinY, tempMin, Fonts::loadMetric,
                                    kTempColor);
}

void WeatherWidget::drawLabeledValue(int16_t bandTop, int16_t bandBottom, int16_t labelY,
                                     int16_t valueY, const char* label, const char* value,
                                     uint16_t valueColor, uint8_t col) {
    LGFX* lcd = getLcd();
    const int16_t cx = columnCenter(col);
    clearCellBand(col, bandTop, bandBottom);

    // Label above the value.
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + labelY, label, Fonts::loadLabel,
                                    kLabelColor);

    // Value below.
    WidgetPainter::drawCenteredText(lcd, cx, dimensions_.y + valueY, value, Fonts::loadMetric,
                                    valueColor);
}

void WeatherWidget::drawNoData() {
    LGFX* lcd = getLcd();
    WidgetPainter::drawCenteredCell(lcd, dimensions_.x, dimensions_.y, dimensions_.width,
                                    dimensions_.height, UiText::kNoData, Fonts::loadLabel,
                                    TFT_DARKGREY);
}