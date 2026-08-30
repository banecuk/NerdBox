#include "RoomClimateWidget.h"

#include <cstdio>

#include "ui/core/UiText.h"
#include "ui/widgets/base/WidgetPainter.h"
#include "ui/widgets/display/WeatherFormat.h"

RoomClimateWidget::RoomClimateWidget(const WidgetInterface::Dimensions& dims,
                                     uint32_t updateIntervalMs,
                                     const RoomClimateData& roomClimateData)
    : Widget(dims, updateIntervalMs), roomClimateData_(roomClimateData) {}

void RoomClimateWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    lastAvail_ = false;
    lastTempX10_ = INT16_MIN;
    lastHumidity_ = 0xFF;
}

void RoomClimateWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    if (!freshness_.isFresh()) {
        if (forceRedraw || lastAvail_) {
            drawNoData();
            lastAvail_ = false;
        }
        return;
    }

    const bool becomingAvailable = !lastAvail_;
    const bool tempChanged =
        forceRedraw || becomingAvailable || roomClimateData_.temperature_x10 != lastTempX10_;
    const bool humidityChanged =
        forceRedraw || becomingAvailable || roomClimateData_.humidity != lastHumidity_;

    if (!tempChanged && !humidityChanged)
        return;

    if (becomingAvailable)
        drawStatic();

    if (tempChanged) {
        char bufTemp[8];
        formatX10OneDecimal(bufTemp, sizeof(bufTemp), roomClimateData_.temperature_x10);
        clearCell(false);
        WidgetPainter::drawValueWithUnit(getLcd(), dimensions_.x + dimensions_.width / 2,
                                         rowCenterY(false), bufTemp,
                                         "\xc2\xb0"
                                         "C",
                                         TFT_WHITE, kUnitColor);
    }

    if (humidityChanged) {
        char bufHumidity[8];
        snprintf(bufHumidity, sizeof(bufHumidity), "%d",
                 static_cast<int>(roomClimateData_.humidity));
        clearCell(true);
        WidgetPainter::drawValueWithUnit(getLcd(), dimensions_.x + dimensions_.width / 2,
                                         rowCenterY(true), bufHumidity, "%", kHumidityColor,
                                         kUnitColor);
    }

    lastAvail_ = true;
    lastTempX10_ = roomClimateData_.temperature_x10;
    lastHumidity_ = roomClimateData_.humidity;
}

int16_t RoomClimateWidget::rowCenterY(bool bottomRow) const {
    if (bottomRow) {
        return dimensions_.y + dimensions_.height - 20;
    }
    return dimensions_.y + 10;
}

void RoomClimateWidget::clearCell(bool bottomRow) {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    const int16_t y = dimensions_.y + (bottomRow ? dimensions_.height / 2 : 0);
    lcd->fillRect(dimensions_.x, y, dimensions_.width, dimensions_.height / 2, TFT_BLACK);
}

void RoomClimateWidget::drawNoData() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, TFT_BLACK);

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, TFT_BLACK);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(UiText::kNoData, dimensions_.x + dimensions_.width / 2,
                    dimensions_.y + dimensions_.height / 2);
    Fonts::unload(lcd);
}
