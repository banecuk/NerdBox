#pragma once

#include "core/resources/FontRegistry.h"
#include "core/resources/weather_outlines_44.h"
#include "services/airQuality/AirQualityData.h"
#include "ui/widgets/base/Widget.h"

// Full-width bar displayed below PcMetricsWidget on the main screen.
//
// Layout (480 px wide, 44 px tall):
//
//   [icon 44px] | [°C]      | [hu%] | [hPa] | [m/s] | [AQI nnn]
//     44 px        ~87 px     ~87px   ~87px   ~87px     ~87 px
//
// Icon: 44×44 px, full widget height, from weather_outlines_44.h.
// Remaining 436 px split into 5 equal tiles of 87 px (remainder 1 px on last).
// Values use loadMetric() — NotoSans 18 pt.
// "AQI" is a dim inline prefix.
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
    static constexpr uint16_t kIconW     = 44;
    static constexpr uint8_t  kTileCount = 5;
    static constexpr uint16_t kTileArea  = 480 - kIconW;           // 436
    static constexpr uint16_t kTileW     = kTileArea / kTileCount; // 87

    // -----------------------------------------------------------------------
    const AirQualityData& airData_;

    // Cached values for dirty detection
    int8_t   lastTemp_     = -128;
    uint8_t  lastHumidity_ = 0xFF;
    int16_t  lastPressure_ = -1;
    uint16_t lastWindX10_  = 0xFFFF;
    uint8_t  lastAqi_      = 0xFF;
    bool     lastAvail_    = false;
    char     lastIcon_[4]  = {0};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void drawTile(uint8_t tileIndex, const char* prefix,
                  const char* value, uint16_t valueColor);
    void drawIcon(const char* code);
    void drawNoData();
    const uint16_t* iconForCode(const char* code) const;
    uint16_t aqiColor(uint8_t aqi) const;
};
