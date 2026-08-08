#pragma once

#include <functional>

#include "core/events/EventTypes.h"
#include "core/resources/FontRegistry.h"
#include "core/resources/weather_icons_44.h"
#include "services/airQuality/AirQualityData.h"
#include "services/airQuality/AirQualityService.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Four-column block shown to the right of ThreadsWidget on the main screen.
//
// Layout (right-aligned band, 240 px wide):
//
//   col 1        col 2        col 3       col 4
//  [icon 44px]  [ 22 °C ]    [1013 hPa]   [  AQI ]
//               [ 55 %  ]    [ 3.4 m/s]   [  42  ]
//
// Column 1: 44×44 weather symbol, vertically centred.
// Columns 2–4: two rows each — temperature/humidity, pressure/wind speed, and
// a fixed "AQI" label over its dynamically coloured value.
// No separators between columns and no border around the widget.
// Numeric values use loadMetric() — NotoSans 18 pt; units ("°C", "%", " hPa",
// " m/s") are drawn in a darker shade than their value, and the "AQI" label
// uses loadLabel() — NotoSansDisplay 12 pt.
class AirQualityWidget : public Widget {
public:
    using ActionCallback = std::function<void(EventType)>;

    AirQualityWidget(const WidgetInterface::Dimensions& dims,
                     uint32_t updateIntervalMs,
                     const AirQualityData& airData,
                     EventType action = EventType::NONE,
                     ActionCallback callback = nullptr);

    bool handleTouch(uint16_t x, uint16_t y) override;

protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

private:
    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    static constexpr uint16_t kColWidth[4] = {44, 52, 88, 56};

    static constexpr uint16_t kIconW = kColWidth[0];

    // Column 1 x-offset; column 2 starts after column 1, etc.
    static constexpr uint16_t kCol2X = kColWidth[0];
    static constexpr uint16_t kCol3X = kColWidth[0] + kColWidth[1];
    static constexpr uint16_t kCol4X = kColWidth[0] + kColWidth[1] + kColWidth[2];

    static constexpr uint16_t kColCenter[4] = {kColWidth[0] / 2,
                                               kCol2X + kColWidth[1] / 2,
                                               kCol3X + kColWidth[2] / 2,
                                               kCol4X + kColWidth[3] / 2};

    // Unit text colour — same as the "AQI" label (0x8410) so every unit/label
    // on the widget shares one dim shade beneath its value.
    static constexpr uint16_t kUnitColor = 0x8410;

    // -----------------------------------------------------------------------
    const AirQualityData& airData_;

    // Data is sticky-available (AirQualityService never clears is_available
    // on fetch failure) — freshness must be checked by age too, or a stale
    // outage-era reading displays forever with no indication.
    DataFreshnessGuard<bool, unsigned long> freshness_{airData_.is_available, airData_.last_update,
                                                        AirQualityService::kRefreshIntervalMs};

    // Optional tap action (mirrors FpsWidget/ButtonWidget): when a callback
    // is set, a tap publishes `action_`, e.g. to open the Weather screen.
    EventType action_;
    ActionCallback callback_;

    // Cached values for dirty detection
    int8_t   lastTemp_     = -128;
    uint8_t  lastHumidity_ = 0xFF;
    int16_t  lastPressure_ = -1;
    uint16_t lastWindX10_  = 0xFFFF;
    uint16_t lastAqi_      = 0xFFFF;
    bool     lastAvail_    = false;
    char     lastIcon_[4]  = {0};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Centre Y (widget-relative) of the top / bottom row of a two-row column.
    int16_t rowCenterY(bool bottomRow) const;

    // Draws a centred string at the given column's centre on a row centre.
    // Numeric cells use the metric font, the "AQI" label the label font.
    void drawCellText(uint8_t col, bool bottomRow, const char* text, uint16_t color,
                      bool labelFont);

    // Draws a value followed by its unit, centred as a pair in the cell. The
    // value uses the metric font in `valueColor`; the unit uses the label font
    // in kUnitColor. A null/empty unit draws just the value.
    void drawValueWithUnit(uint8_t col, bool bottomRow, const char* value, const char* unit,
                           uint16_t valueColor);

    void drawIcon(const char* code);
    void drawNoData();
    const uint16_t* iconForCode(const char* code) const;
    uint16_t aqiColor(uint16_t aqi) const;
};
