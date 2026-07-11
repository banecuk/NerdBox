#include "AirQualityWidget.h"

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AirQualityWidget::AirQualityWidget(const WidgetInterface::Dimensions& dims,
                                   uint32_t updateIntervalMs,
                                   const AirQualityData& airData)
    : Widget(dims, updateIntervalMs),
      airData_(airData) {}

// ---------------------------------------------------------------------------
// drawStatic
// ---------------------------------------------------------------------------

void AirQualityWidget::drawStatic() {
    if (!isInitialized_ || !getLcd()) return;

    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    lcd->drawFastHLine(dimensions_.x, dimensions_.y,
                       dimensions_.width, 0x2104);

    // Tile separators (no separator between icon and first tile)
    for (uint8_t i = 1; i < kTileCount; ++i) {
        const int16_t sx = dimensions_.x + kIconW + i * kTileW;
        lcd->drawFastVLine(sx, dimensions_.y + 2, dimensions_.height - 4, 0x2104);
    }

    isStaticDrawn_ = true;

    lastAvail_    = false;
    lastTemp_     = -128;
    lastHumidity_ = 0xFF;
    lastPressure_ = -1;
    lastWindX10_  = 0xFFFF;
    lastAqi_      = 0xFF;
    lastIcon_[0]  = '\0';

    clearDirty();
}

// ---------------------------------------------------------------------------
// onDraw
// ---------------------------------------------------------------------------

void AirQualityWidget::onDraw(bool forceRedraw) {
    if (!getLcd()) return;

    if (!airData_.is_available) {
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

    // Icon
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

    const TileSpec tiles[kTileCount] = {
        {nullptr, bufTemp, TFT_WHITE, "\xc2\xb0""C"},
        {nullptr, bufHumidity, 0x867F, "%"},
        {nullptr, bufPressure, TFT_LIGHTGREY, " hPa"},
        {nullptr, bufWind, TFT_LIGHTGREY, " m/s"},
        {"AQI ", bufAqi, aqiColor(airData_.aqi_us), nullptr},
    };
    const bool tileChanged[kTileCount] = {
        tempChanged, humidityChanged, pressureChanged, windChanged, aqiChanged,
    };
    drawTiles(tiles, tileChanged);

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
// drawIcon
// ---------------------------------------------------------------------------

void AirQualityWidget::drawIcon(const char* code) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    lcd->fillRect(dimensions_.x, dimensions_.y, kIconW, dimensions_.height, TFT_BLACK);

    const uint16_t* data = iconForCode(code);
    if (data) {
        lcd->pushImage(dimensions_.x, dimensions_.y,
                       kIconW, kIconW, data);
    }
}

// ---------------------------------------------------------------------------
// drawTiles
// Values use loadMetric() — NotoSans 18 pt.
// prefix/unit use loadLabel() — NotoSansDisplay 12 pt, dimmed to de-emphasise.
//
// Two passes across all tiles per font (measure, then draw) instead of
// loading/unloading per tile — loadFont() streams the font from PROGMEM into
// a fresh heap buffer each time, so this keeps font swaps at a flat 4
// (2 measure + 2 draw) regardless of tile count, instead of up to 4 per tile.
// ---------------------------------------------------------------------------

void AirQualityWidget::drawTiles(const TileSpec (&tiles)[kTileCount],
                                 const bool (&changed)[kTileCount]) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    const int16_t ty = dimensions_.y;
    const int16_t h  = dimensions_.height;
    const int16_t midY = ty + h / 2;

    for (uint8_t i = 0; i < kTileCount; ++i) {
        if (!changed[i]) continue;
        const int16_t tx = dimensions_.x + kIconW + i * kTileW;
        lcd->fillRect(tx + 1, ty + 1, kTileW - 2, h - 2, TFT_BLACK);
    }

    int16_t valW[kTileCount]  = {0};
    int16_t textW[kTileCount] = {0};

    // Pass 1 — measure values (needed for every branch except value-only,
    // which uses MC_DATUM and doesn't need a pre-measured width).
    Fonts::loadMetric(lcd);
    for (uint8_t i = 0; i < kTileCount; ++i) {
        if (!changed[i]) continue;
        if (tiles[i].prefix != nullptr || tiles[i].unit != nullptr) {
            valW[i] = static_cast<int16_t>(lcd->textWidth(tiles[i].value));
        }
    }
    Fonts::unload(lcd);

    // Pass 2 — measure prefix/unit text.
    Fonts::loadLabel(lcd);
    for (uint8_t i = 0; i < kTileCount; ++i) {
        if (!changed[i]) continue;
        const char* text = tiles[i].prefix != nullptr ? tiles[i].prefix : tiles[i].unit;
        if (text != nullptr) {
            textW[i] = static_cast<int16_t>(lcd->textWidth(text));
        }
    }
    Fonts::unload(lcd);

    // Pass 3 — draw values.
    Fonts::loadMetric(lcd);
    for (uint8_t i = 0; i < kTileCount; ++i) {
        if (!changed[i]) continue;
        const int16_t tx = dimensions_.x + kIconW + i * kTileW;
        const int16_t cx = tx + kTileW / 2;
        lcd->setTextColor(tiles[i].valueColor, TFT_BLACK);

        if (tiles[i].prefix != nullptr) {
            const int16_t startX = cx - (textW[i] + valW[i]) / 2;
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(tiles[i].value, startX + textW[i], midY);
        } else if (tiles[i].unit != nullptr) {
            const int16_t startX = cx - (valW[i] + textW[i]) / 2;
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(tiles[i].value, startX, midY);
        } else {
            lcd->setTextDatum(MC_DATUM);
            lcd->drawString(tiles[i].value, cx, midY);
        }
    }
    Fonts::unload(lcd);

    // Pass 4 — draw prefix/unit text.
    Fonts::loadLabel(lcd);
    lcd->setTextDatum(ML_DATUM);
    for (uint8_t i = 0; i < kTileCount; ++i) {
        if (!changed[i]) continue;
        const int16_t tx = dimensions_.x + kIconW + i * kTileW;
        const int16_t cx = tx + kTileW / 2;

        if (tiles[i].prefix != nullptr) {
            const int16_t startX = cx - (textW[i] + valW[i]) / 2;
            lcd->setTextColor(0x8410, TFT_BLACK);
            lcd->drawString(tiles[i].prefix, startX, midY);
        } else if (tiles[i].unit != nullptr) {
            const int16_t startX = cx - (valW[i] + textW[i]) / 2;
            lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
            lcd->drawString(tiles[i].unit, startX + valW[i], midY);
        }
    }
    Fonts::unload(lcd);
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

uint16_t AirQualityWidget::aqiColor(uint8_t aqi) const {
    if (aqi <= 50)  return 0x07E0;
    if (aqi <= 100) return 0xFFE0;
    if (aqi <= 150) return 0xFD20;
    if (aqi <= 200) return 0xF800;
    return 0xF81F;
}

bool AirQualityWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
