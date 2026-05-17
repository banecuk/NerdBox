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
// drawStatic — background + chrome, called once on first paint
// ---------------------------------------------------------------------------

void AirQualityWidget::drawStatic() {
    if (!isInitialized_ || !getLcd()) return;

    LGFX* lcd = getLcd();

    // Full background
    lcd->fillRect(dimensions_.x, dimensions_.y,
                  dimensions_.width, dimensions_.height, TFT_BLACK);

    // Top separator line
    lcd->drawFastHLine(dimensions_.x, dimensions_.y,
                       dimensions_.width, 0x2104 /* dark grey */);

    // Separator between icon area and first tile
    lcd->drawFastVLine(dimensions_.x + kIconW,
                       dimensions_.y + 2, dimensions_.height - 4, 0x2104);

    // Separators between the 5 tiles
    for (uint8_t i = 1; i < kTileCount; ++i) {
        const int16_t sx = dimensions_.x + kIconW + i * kTileW;
        lcd->drawFastVLine(sx, dimensions_.y + 2, dimensions_.height - 4, 0x2104);
    }

    isStaticDrawn_ = true;

    // Force full repaint next onDraw
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

    // If recovering from no-data, redraw the static chrome first
    if (!lastAvail_) drawStatic();

    LGFX* lcd = getLcd();
    char buf[20];

    // --- Weather icon (far left, 36×36) ---
    if (forceRedraw || !lastAvail_ || iconChanged) {
        drawIcon(airData_.icon_code);
    }

    // --- Tile 0: Temperature (main) + heat index (sub) ---
    snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", static_cast<int>(airData_.temperature));
    if (airData_.heat_index != airData_.temperature) {
        char sub[12];
        snprintf(sub, sizeof(sub), "hi:%d\xc2\xb0""C", static_cast<int>(airData_.heat_index));
        drawTile(0, nullptr, buf, TFT_WHITE, sub);
    } else {
        drawTile(0, nullptr, buf, TFT_WHITE, nullptr);
    }

    // --- Tile 1: Humidity ---
    snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(airData_.humidity));
    drawTile(1, nullptr, buf, 0x867F /* sky blue */);

    // --- Tile 2: Pressure ---
    snprintf(buf, sizeof(buf), "%dhPa", static_cast<int>(airData_.pressure));
    drawTile(2, nullptr, buf, TFT_LIGHTGREY);

    // --- Tile 3: Wind speed ---
    snprintf(buf, sizeof(buf), "%u.%um/s",
             airData_.wind_speed_x10 / 10,
             airData_.wind_speed_x10 % 10);
    drawTile(3, nullptr, buf, TFT_LIGHTGREY);

    // --- Tile 4: AQI — "AQI" as dim inline prefix ---
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
// drawIcon — pushImage for the 36×36 PROGMEM sprite
// ---------------------------------------------------------------------------

void AirQualityWidget::drawIcon(const char* code) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    const int16_t ix = dimensions_.x;
    const int16_t iy = dimensions_.y;

    lcd->fillRect(ix, iy, kIconW, dimensions_.height, TFT_BLACK);

    const uint16_t* data = iconForCode(code);
    if (data) {
        // pushImage with transparent colour 0x0000 (black background in icons)
        lcd->pushImage(ix, iy, kIconW, kIconW, data, static_cast<uint16_t>(0x0000));
    }
}

// ---------------------------------------------------------------------------
// drawTile — clear tile interior then draw prefix + value (+ optional sub)
// ---------------------------------------------------------------------------

void AirQualityWidget::drawTile(uint8_t tileIndex, const char* prefix,
                                const char* value, uint16_t valueColor,
                                const char* sub) {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    const int16_t tx = dimensions_.x + kIconW + tileIndex * kTileW;
    const int16_t ty = dimensions_.y;
    const int16_t cx = tx + kTileW / 2;
    const int16_t h  = dimensions_.height;

    // Clear
    lcd->fillRect(tx + 1, ty + 1, kTileW - 2, h - 2, TFT_BLACK);

    if (sub != nullptr) {
        // Two rows: value in upper ~60%, sub label in lower ~40%
        const int16_t valueY = ty + h * 2 / 5;
        const int16_t subY   = ty + h - 4;

        // Main value
        Fonts::loadValue(lcd);
        lcd->setTextDatum(MC_DATUM);

        if (prefix != nullptr) {
            // Measure prefix width to left-align the combined string
            Fonts::loadLabel(lcd);
            const int16_t prefW = static_cast<int16_t>(lcd->textWidth(prefix));
            Fonts::unload(lcd);

            Fonts::loadLabel(lcd);
            lcd->setTextColor(0x4208 /* dim grey */, TFT_BLACK);
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(prefix, cx - kTileW / 2 + 4, valueY);
            Fonts::unload(lcd);

            Fonts::loadValue(lcd);
            lcd->setTextColor(valueColor, TFT_BLACK);
            lcd->setTextDatum(ML_DATUM);
            lcd->drawString(value, cx - kTileW / 2 + 4 + prefW, valueY);
            Fonts::unload(lcd);
        } else {
            Fonts::loadValue(lcd);
            lcd->setTextColor(valueColor, TFT_BLACK);
            lcd->setTextDatum(MC_DATUM);
            lcd->drawString(value, cx, valueY);
            Fonts::unload(lcd);
        }

        // Sub label (heat index)
        Fonts::loadLabel(lcd);
        lcd->setTextColor(0x4208 /* dim grey */, TFT_BLACK);
        lcd->setTextDatum(BC_DATUM);
        lcd->drawString(sub, cx, subY);
        Fonts::unload(lcd);

    } else if (prefix != nullptr) {
        // Inline prefix + value, vertically centred
        const int16_t midY = ty + h / 2;

        Fonts::loadLabel(lcd);
        const int16_t prefW = static_cast<int16_t>(lcd->textWidth(prefix));
        Fonts::unload(lcd);

        Fonts::loadValue(lcd);
        const int16_t valW = static_cast<int16_t>(lcd->textWidth(value));
        Fonts::unload(lcd);

        const int16_t totalW  = prefW + valW;
        const int16_t startX  = cx - totalW / 2;

        Fonts::loadLabel(lcd);
        lcd->setTextColor(0x4208 /* dim grey */, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(prefix, startX, midY);
        Fonts::unload(lcd);

        Fonts::loadValue(lcd);
        lcd->setTextColor(valueColor, TFT_BLACK);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(value, startX + prefW, midY);
        Fonts::unload(lcd);

    } else {
        // Value only — centred
        Fonts::loadValue(lcd);
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
// iconForCode — maps a 3-char AirVisual icon code to its PROGMEM array
// ---------------------------------------------------------------------------

const uint16_t* AirQualityWidget::iconForCode(const char* code) const {
    if (!code || code[0] == '\0') return nullptr;

    // Map: code string → PROGMEM array
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

    return nullptr;  // Unknown code — leave icon area empty
}

// ---------------------------------------------------------------------------
// aqiColor
// ---------------------------------------------------------------------------

uint16_t AirQualityWidget::aqiColor(uint8_t aqi) const {
    if (aqi <= 50)  return 0x07E0;  // Green   — Good
    if (aqi <= 100) return 0xFFE0;  // Yellow  — Moderate
    if (aqi <= 150) return 0xFD20;  // Orange  — USG
    if (aqi <= 200) return 0xF800;  // Red     — Unhealthy
    return 0xF81F;                  // Magenta — Very Unhealthy/Hazardous
}

bool AirQualityWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    return false;
}
