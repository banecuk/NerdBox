#pragma once

#include "core/resources/FontRegistry.h"
#include "core/resources/weather_icons.h"
#include "services/airQuality/AirQualityData.h"
#include "ui/widgets/base/Widget.h"

// Full-width bar displayed below PcMetricsWidget on the main screen.
//
// Layout (480 px wide, 36 px tall):
//
//   [icon 36px] | [°C  hi:°C] | [hu%] | [hPa] | [m/s] | [AQI nnn]
//     36 px          ~89 px     ~89px   ~89px   ~89px     ~78 px
//
// The icon occupies the first 36 px (full widget height).
// The remaining 444 px are split into 5 equal tiles of ~88 px.
// The AQI tile shows "AQI" as a dim prefix on the left of the value
// rather than a separate row, saving vertical space.
//
// When no data is available a dim "NO DATA" placeholder fills the bar.
class AirQualityWidget : public Widget {
public:
    AirQualityWidget(const WidgetInterface::Dimensions& dims,
                     uint32_t updateIntervalMs,
                     const AirQualityData& airData);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

protected:
    void onDraw(bool forceRedraw) override;

private:
    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    static constexpr uint16_t kIconW    = 36;   // weather icon width = height
    static constexpr uint8_t  kTileCount = 5;
    // Tiles start after the icon; distribute remaining width evenly.
    // 480 - 36 = 444 px / 5 = 88 px per tile (remainder goes to last tile).
    static constexpr uint16_t kTileArea = 480 - kIconW;          // 444
    static constexpr uint16_t kTileW    = kTileArea / kTileCount; // 88

    // -----------------------------------------------------------------------
    const AirQualityData& airData_;

    // Cached values for dirty detection
    int8_t   lastTemp_     = -128;
    int8_t   lastHeatIdx_  = -128;
    uint8_t  lastHumidity_ = 0xFF;
    int16_t  lastPressure_ = -1;
    uint16_t lastWindX10_  = 0xFFFF;
    uint8_t  lastAqi_      = 0xFF;
    bool     lastAvail_    = false;
    char     lastIcon_[4]  = {0};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    // Draw one of the 5 data tiles (index 0-4).
    // prefix: optional dim text drawn left of the value (e.g. "AQI ").
    // value:  main value string.
    // sub:    optional small text on the bottom half (e.g. heat index).
    void drawTile(uint8_t tileIndex, const char* prefix,
                  const char* value, uint16_t valueColor,
                  const char* sub = nullptr);

    void drawIcon(const char* code);
    void drawNoData();

    // Returns the PROGMEM pointer for a given icon code, or nullptr.
    const uint16_t* iconForCode(const char* code) const;

    // AQI → RGB565 colour
    uint16_t aqiColor(uint8_t aqi) const;
};
