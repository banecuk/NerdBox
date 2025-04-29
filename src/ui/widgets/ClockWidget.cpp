#include "ClockWidget.h"

#include <stdio.h>
#include <time.h>

ClockWidget::ClockWidget(const Dimensions& dims, uint32_t updateIntervalMs,
                         uint16_t textColor, uint16_t bgColor, uint8_t textSize,
                         const std::string& format)
    : Widget(dims, updateIntervalMs),  // Call base constructor
      textColor_(textColor),
      bgColor_(bgColor),
      textSize_(textSize),
      format_(format) {}

void ClockWidget::draw(bool forceRedraw /* = false */) {
    if (!initialized_ || !lcd_) return;

    struct tm timeinfo;
    getLocalTime(&timeinfo, 5);

    bool needsRedraw = forceRedraw;

    if (timeinfo.tm_sec != lastDisplayedSecond_) {
        needsRedraw = true;
    }

    if (needsRedraw) {
        char timeBuffer[32];

        lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width,
                       dimensions_.height, bgColor_);

        lcd_->setTextColor(textColor_, bgColor_);
        lcd_->setTextSize(textSize_);
        lcd_->setTextDatum(MC_DATUM);

        strftime(timeBuffer, sizeof(timeBuffer), format_.c_str(), &timeinfo);
        lastDisplayedSecond_ = timeinfo.tm_sec;  // Store the second we just drew

        uint16_t textX = dimensions_.x + dimensions_.width / 2;
        uint16_t textY = dimensions_.y + dimensions_.height / 2;

        lcd_->drawString(timeBuffer, textX, textY);
        lcd_->setTextSize(1);

        lastUpdateTimeMs_ = millis();
    }
}

bool ClockWidget::handleTouch(uint16_t x, uint16_t y) { return false; }