#pragma once

#include "services/roomClimate/RoomClimateData.h"
#include "services/roomClimate/RoomClimateService.h"
#include "ui/widgets/base/Widget.h"
#include "utils/DataFreshnessGuard.h"

// Local room temperature/humidity readout — a single 64x56 column sitting
// between ThreadsWidget and AirQualityWidget on the main screen's top band.
// Same two-row style as AirQualityWidget's temp/humidity column so the two
// blocks read as one band:
//
//   [ 23.4 °C ]
//   [   45 %  ]
//
// One decimal place, always shown (20.0, not 20): this is an indoor sensor,
// so it's never negative and never more than 2 integer digits — worst case
// "99.9°C" is narrower than AirQualityWidget's signed outdoor equivalent, but
// still needs the full 64px column, not RoomClimateWidget's original
// integer-only 44px cut. No border, no separators, black background, no tap
// action.
class RoomClimateWidget : public Widget {
 public:
    RoomClimateWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                      const RoomClimateData& roomClimateData);

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // Unit text colour — same dim shade AirQualityWidget uses for every
    // unit/label on its band.
    static constexpr uint16_t kUnitColor = 0x8410;
    // Humidity value colour — matches AirQualityWidget's humidity cyan.
    static constexpr uint16_t kHumidityColor = 0x867F;

    const RoomClimateData& roomClimateData_;

    DataFreshnessGuard freshness_{roomClimateData_.freshness, RoomClimateService::kStaleTimeoutMs};

    // Cached values for dirty detection
    int16_t lastTempX10_ = INT16_MIN;
    uint8_t lastHumidity_ = 0xFF;
    bool lastAvail_ = false;

    // Centre Y (widget-relative) of the top / bottom row of the two-row
    // column — mirrors AirQualityWidget::rowCenterY so the two blocks share
    // the same row placement.
    int16_t rowCenterY(bool bottomRow) const;

    // Blacks out the half-height cell before drawing new text into it — same
    // rationale as AirQualityWidget::clearCell.
    void clearCell(bool bottomRow);

    void drawNoData();
};
