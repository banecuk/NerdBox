#include "AirQualityWidget.h"

#include <cstdio>
#include <cstring>

#include "ui/core/Colors.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AirQualityWidget::AirQualityWidget(const WidgetInterface::Dimensions& dims,
                                   uint32_t updateIntervalMs,
                                   const AirQualityData& airData,
                                   EventType action,
                                   ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      airData_(airData),
      action_(action),
      callback_(std::move(callback)) {}

// ---------------------------------------------------------------------------
// drawStatic
// ---------------------------------------------------------------------------

void AirQualityWidget::onDrawStatic() {
    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    lastAvail_    = false;
    lastTemp_     = -128;
    lastHumidity_ = 0xFF;
    lastPressure_ = -1;
    lastWindX10_  = 0xFFFF;
    lastAqi_      = 0xFF;
    lastIcon_[0]  = '\0';
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void AirQualityWidget::onDraw(bool forceRedraw) {
    if (!getLcd()) return;

    if (!freshness_.isFresh()) {
        if (forceRedraw || lastAvail_) {
            drawNoData();
            lastAvail_ = false;
        }
        return;
    }

    const bool iconChanged = (strncmp(airData_.icon_code, lastIcon_,
                                      sizeof(lastIcon_)) != 0);
    const bool becomingAvailable = !lastAvail_;

    const bool tempChanged     = forceRedraw || becomingAvailable ||
                                 airData_.temperature != lastTemp_;
    const bool humidityChanged = forceRedraw || becomingAvailable ||
                                 airData_.humidity != lastHumidity_;
    const bool pressureChanged = forceRedraw || becomingAvailable ||
                                 airData_.pressure != lastPressure_;
    const bool windChanged     = forceRedraw || becomingAvailable ||
                                 airData_.wind_speed_x10 != lastWindX10_;
    const bool aqiChanged      = forceRedraw || becomingAvailable ||
                                 airData_.aqi_us != lastAqi_;

    const bool changed = tempChanged || humidityChanged || pressureChanged ||
                         windChanged || aqiChanged || iconChanged;

    if (!changed) return;

    if (becomingAvailable) drawStatic();

    if (forceRedraw || becomingAvailable || iconChanged) {
        drawIcon(airData_.icon_code);
    }

    char bufTemp[8], bufHumidity[8], bufPressure[8], bufWind[8], bufAqi[8];
    snprintf(bufTemp, sizeof(bufTemp), "%d", static_cast<int>(airData_.temperature));
    snprintf(bufHumidity, sizeof(bufHumidity), "%d", static_cast<int>(airData_.humidity));
    snprintf(bufPressure, sizeof(bufPressure), "%d", static_cast<int>(airData_.pressure));
    snprintf(bufWind, sizeof(bufWind), "%u.%u", airData_.wind_speed_x10 / 10,
             airData_.wind_speed_x10 % 10);
    snprintf(bufAqi, sizeof(bufAqi), "%d", static_cast<int>(airData_.aqi_us));

    if (tempChanged)
        drawValueWithUnit(1, false, bufTemp, "\xc2\xb0" "C", TFT_WHITE);
    if (humidityChanged)
        drawValueWithUnit(1, true, bufHumidity, "%", 0x867F);
    if (pressureChanged)
        drawValueWithUnit(2, false, bufPressure, " hPa", TFT_LIGHTGREY);
    if (windChanged)
        drawValueWithUnit(2, true, bufWind, " m/s", TFT_LIGHTGREY);

    drawCellText(3, false, "AQI", 0x8410, true);
    if (aqiChanged)
        drawValueWithUnit(3, true, bufAqi, "", aqiColor(airData_.aqi_us));

    // Cache
    lastAvail_    = true;
    lastTemp_     = airData_.temperature;
    lastHumidity_ = airData_.humidity;
    lastPressure_ = airData_.pressure;
    lastWindX10_  = airData_.wind_speed_x10;
    lastAqi_      = airData_.aqi_us;
    strncpy(lastIcon_, airData_.icon_code, sizeof(lastIcon_) - 1);
    lastIcon_[sizeof(lastIcon_) - 1] = '\0';
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

int16_t AirQualityWidget::rowCenterY(bool bottomRow) const {
    // Compact, top-anchored rows: no top padding, a small bottom inset, and a
    // tighter gap than the old fraction-based (30% / 72%) placement.
    if (bottomRow) {
        return dimensions_.y + dimensions_.height - 20;
    }
    return dimensions_.y + 10;
}

void AirQualityWidget::drawCellText(uint8_t col, bool bottomRow, const char* text,
                                    uint16_t color, bool labelFont) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    if (labelFont) {
        Fonts::loadLabel(lcd);
    } else {
        Fonts::loadMetric(lcd);
    }

    const int16_t cx = dimensions_.x + kColCenter[col];
    lcd->setTextColor(color, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(text, cx, rowCenterY(bottomRow));

    Fonts::unload(lcd);
}

void AirQualityWidget::drawValueWithUnit(uint8_t col, bool bottomRow, const char* value,
                                         const char* unit, uint16_t valueColor) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    const int16_t cx = dimensions_.x + kColCenter[col];
    const int16_t cy = rowCenterY(bottomRow);

    if (!unit || unit[0] == '\0') {
        Fonts::loadMetric(lcd);
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(value, cx, cy);
        Fonts::unload(lcd);
        return;
    }

    int16_t valueW, valueH, unitW;
    Fonts::loadMetric(lcd);
    valueW = static_cast<int16_t>(lcd->textWidth(value));
    valueH = static_cast<int16_t>(lcd->fontHeight());
    Fonts::unload(lcd);
    Fonts::loadLabel(lcd);
    unitW = static_cast<int16_t>(lcd->textWidth(unit));
    Fonts::unload(lcd);

    const int16_t startX = cx - (valueW + unitW) / 2;
    // Draw both on the same baseline so the unit sits at the bottom of the
    // value digits. The baseline of a MC_DATUM draw is ~half the value's own
    // glyph height below its vertical centre.
    const int16_t baselineY = cy + valueH / 2;

    Fonts::loadMetric(lcd);
    lcd->setTextColor(valueColor, TFT_BLACK);
    lcd->setTextDatum(L_BASELINE);
    lcd->drawString(value, startX, baselineY);
    Fonts::unload(lcd);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(kUnitColor, TFT_BLACK);
    lcd->setTextDatum(L_BASELINE);
    lcd->drawString(unit, startX + valueW, baselineY);
    Fonts::unload(lcd);
}

// ---------------------------------------------------------------------------
// drawIcon
// ---------------------------------------------------------------------------

void AirQualityWidget::drawIcon(const char* code) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    lcd->fillRect(dimensions_.x, dimensions_.y, kIconW, dimensions_.height, TFT_BLACK);

    const uint16_t* data = iconForCode(code);
    if (data) {
        const int16_t y = dimensions_.y + (dimensions_.height - kIconW) / 2;
        lcd->pushImage(dimensions_.x, y, kIconW, kIconW, data);
    }
}

// ---------------------------------------------------------------------------
// drawNoData
// ---------------------------------------------------------------------------

void AirQualityWidget::drawNoData() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString("NO DATA",
                    dimensions_.x + dimensions_.width  / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}

// ---------------------------------------------------------------------------
// iconForCode
// ---------------------------------------------------------------------------

const uint16_t* AirQualityWidget::iconForCode(const char* code) const {
    if (!code || code[0] == '\0') return nullptr;
    if (strcmp(code, "01d") == 0) return icon_01d;
    if (strcmp(code, "01n") == 0) return icon_01n;
    if (strcmp(code, "02d") == 0) return icon_02d;
    if (strcmp(code, "02n") == 0) return icon_02n;
    if (strcmp(code, "03d") == 0) return icon_03d;
    if (strcmp(code, "03n") == 0) return icon_03d;  // reuse day icon for night variant
    if (strcmp(code, "04d") == 0) return icon_04d;
    if (strcmp(code, "04n") == 0) return icon_04d;  // reuse day icon for night variant
    if (strcmp(code, "09d") == 0) return icon_09d;
    if (strcmp(code, "09n") == 0) return icon_09d;  // reuse day icon for night variant
    if (strcmp(code, "10d") == 0) return icon_10d;
    if (strcmp(code, "10n") == 0) return icon_10n;
    if (strcmp(code, "11d") == 0) return icon_11d;
    if (strcmp(code, "11n") == 0) return icon_11d;  // reuse day icon for night variant
    if (strcmp(code, "13d") == 0) return icon_13d;
    if (strcmp(code, "13n") == 0) return icon_13d;  // reuse day icon for night variant
    if (strcmp(code, "50d") == 0) return icon_50d;
    if (strcmp(code, "50n") == 0) return icon_50d;  // reuse day icon for night variant
    return nullptr;
}

// ---------------------------------------------------------------------------
// aqiColor
// ---------------------------------------------------------------------------

uint16_t AirQualityWidget::aqiColor(uint16_t aqi) const {
    if (aqi <= 50)  return 0x07E0;
    if (aqi <= 100) return 0xFFE0;
    if (aqi <= 150) return 0xFD20;
    if (aqi <= 200) return 0xF800;
    return 0xF81F;
}

bool AirQualityWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    if (!callback_) {
        return false;
    }
    callback_(action_);
    return true;
}
