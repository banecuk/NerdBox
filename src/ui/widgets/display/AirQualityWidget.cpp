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
    lastHeatIdx_  = -128;
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
    const bool changed =
        forceRedraw                              ||
        !lastAvail_                              ||
        iconChanged                              ||
        airData_.temperature    != lastTemp_     ||
        airData_.heat_index     != lastHeatIdx_  ||
        airData_.humidity       != lastHumidity_ ||
        airData_.pressure       != lastPressure_ ||
        airData_.wind_speed_x10 != lastWindX10_  ||
        airData_.aqi_us         != lastAqi_;

    if (!changed) return;

    if (!lastAvail_) drawStatic();

    char buf[20];

    // Icon
    if (forceRedraw || !lastAvail_ || iconChanged) {
        drawIcon(airData_.icon_code);
    }

    // Tile 0 — Temperature + optional heat index sub-label
    snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", static_cast<int>(airData_.temperature));
    if (airData_.heat_index != airData_.temperature) {
        char sub[12];
        snprintf(sub, sizeof(sub), "hi:%d\xc2\xb0""C", static_cast<int>(airData_.heat_index));
        drawTile(0, nullptr, buf, TFT_WHITE, sub);
    } else {
        drawTile(0, nullptr, buf, TFT_WHITE);
    }

    // Tile 1 — Humidity
    snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(airData_.humidity));
    drawTile(1, nullptr, buf, 0x867F);

    // Tile 2 — Pressure
    snprintf(buf, sizeof(buf), "%dhPa", static_cast<int>(airData_.pressure));
    drawTile(2, nullptr, buf, TFT_LIGHTGREY);

    // Tile 3 — Wind speed
    snprintf(buf, sizeof(buf), "%u.%um/s",
             airData_.wind_speed_x10 / 10,
             airData_.wind_speed_x10 % 10);
    drawTile(3, nullptr, buf, TFT_LIGHTGREY);

    // Tile 4 — AQI with inline prefix
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(airData_.aqi_us));
    drawTile(4, "AQI ", buf, aqiColor(airData_.aqi_us));

    // Cache
    lastAvail_    = true;
    lastTemp_     = airData_.temperature;
    lastHeatIdx_  = airData_.heat_index;
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
                       kIconW, kIconW, data, static_cast<uint16_t>(0x0000));
    }
}

// ---------------------------------------------------------------------------
// drawTile
// All values use loadMetric() — NotoSans 18 pt.
// prefix uses loadLabel() — NotoSansDisplay 12 pt.
// sub uses loadLabel() — rendered at bottom of tile.
// ---------------------------------------------------------------------------

void AirQualityWidget::drawTile(uint8_t tileIndex, const char* prefix,
                                const char* value, uint16_t valueColor,
                                const char* sub) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    const int16_t tx  = dimensions_.x + kIconW + tileIndex * kTileW;
    const int16_t ty  = dimensions_.y;
    const int16_t cx  = tx + kTileW / 2;
    const int16_t h   = dimensions_.height;

    lcd->fillRect(tx + 1, ty + 1, kTileW - 2, h - 2, TFT_BLACK);

    if (sub != nullptr) {
        // Two rows: metric value in upper ~58%, sub-label at bottom
        const int16_t valueY = ty + h * 11 / 20;  // ~55% down
        const int16_t subY   = ty + h - 3;

        if (prefix != nullptr) {
            Fonts::loadLabel(lcd);
            const int16_t prefW = static_cast<int16_t>(lcd->textWidth(prefix));
            Fonts::unload(lcd);
            Fonts::loadMetric(lcd);
            const int16_t valW = static_cast<int16_t>(lcd->textWidth(value));
            Fonts::unload(lcd);

            const int16_t startX = cx - (prefW + valW) / 2;

            Fonts::loadLabel(lcd);
            lcd->setTextColor(0x8410, TFT_BLACK);
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(prefix, startX, valueY);
            Fonts::unload(lcd);

            Fonts::loadMetric(lcd);
            lcd->setTextColor(valueColor, TFT_BLACK);
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(value, startX + prefW, valueY);
            Fonts::unload(lcd);
        } else {
            Fonts::loadMetric(lcd);
            lcd->setTextColor(valueColor, TFT_BLACK);
            lcd->setTextDatum(MC_DATUM);
            lcd->drawString(value, cx, valueY);
            Fonts::unload(lcd);
        }

        Fonts::loadLabel(lcd);
        lcd->setTextColor(0x8410, TFT_BLACK);
        lcd->setTextDatum(BC_DATUM);
        lcd->drawString(sub, cx, subY);
        Fonts::unload(lcd);

    } else if (prefix != nullptr) {
        // Inline prefix (dim, 12 pt) + value (18 pt), vertically centred
        const int16_t midY = ty + h / 2;

        Fonts::loadLabel(lcd);
        const int16_t prefW = static_cast<int16_t>(lcd->textWidth(prefix));
        Fonts::unload(lcd);
        Fonts::loadMetric(lcd);
        const int16_t valW = static_cast<int16_t>(lcd->textWidth(value));
        Fonts::unload(lcd);

        const int16_t startX = cx - (prefW + valW) / 2;

        Fonts::loadLabel(lcd);
        lcd->setTextColor(0x8410, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(prefix, startX, midY);
        Fonts::unload(lcd);

        Fonts::loadMetric(lcd);
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(value, startX + prefW, midY);
        Fonts::unload(lcd);

    } else {
        // Value only — centred
        Fonts::loadMetric(lcd);
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(value, cx, ty + h / 2);
        Fonts::unload(lcd);
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
    if (strncmp(code, "01d", 3) == 0) return icon_01d;
    if (strncmp(code, "01n", 3) == 0) return icon_01n;
    if (strncmp(code, "02d", 3) == 0) return icon_02d;
    if (strncmp(code, "02n", 3) == 0) return icon_02n;
    if (strncmp(code, "03d", 3) == 0) return icon_03d;
    if (strncmp(code, "04d", 3) == 0) return icon_04d;
    if (strncmp(code, "09d", 3) == 0) return icon_09d;
    if (strncmp(code, "10d", 3) == 0) return icon_10d;
    if (strncmp(code, "10n", 3) == 0) return icon_10n;
    if (strncmp(code, "11d", 3) == 0) return icon_11d;
    if (strncmp(code, "13d", 3) == 0) return icon_13d;
    if (strncmp(code, "50d", 3) == 0) return icon_50d;
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
