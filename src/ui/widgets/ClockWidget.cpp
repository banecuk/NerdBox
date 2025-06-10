#include "ClockWidget.h"

#include <time.h>

ClockWidget::ClockWidget(DisplayContext& context, const Dimensions& dims,
                         uint32_t updateIntervalMs, uint16_t textColor, uint16_t bgColor,
                         uint8_t textSize, const std::string& format)
    : Widget(dims, updateIntervalMs),
      textColor_(textColor),
      bgColor_(bgColor),
      textSize_(textSize),
      format_(format),
      context_(context) {
    // Calculate section widths (assuming format is HH:MM:SS)
    uint16_t totalWidth = dimensions_.width;
    uint16_t partWidth = (totalWidth - 2 * colonWidth_) / 3;  // Equal width for all parts

    // Initialize section positions
    hours_.x = dimensions_.x;
    hours_.width = partWidth;
    hours_.lastValue = -1;

    colon1X_ = hours_.x + hours_.width;

    mins_.x = colon1X_ + colonWidth_;
    mins_.width = partWidth;
    mins_.lastValue = -1;

    colon2X_ = mins_.x + mins_.width;

    secs_.x = colon2X_ + colonWidth_;
    secs_.width = partWidth;
    secs_.lastValue = -1;

    colonY_ = dimensions_.y + dimensions_.height / 2 - (textSize_ * 8) / 2;
}

void ClockWidget::drawStatic() {
    if (!initialized_ || !lcd_) return;

    // Clear entire widget area
    lcd_->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height,
                   bgColor_);

    // Draw static colons
    lcd_->setTextColor(textColor_, bgColor_);
    lcd_->setTextSize(textSize_);
    lcd_->setTextDatum(CL_DATUM);

    lcd_->drawString(":", colon1X_ - 6, colonY_);
    lcd_->drawString(":", colon2X_ - 6, colonY_);

    staticDrawn_ = true;
}

void ClockWidget::draw(bool forceRedraw) {
    if (!initialized_ || !lcd_) return;

    struct tm timeinfo;
    getLocalTime(&timeinfo, 5);

    updateIfNeeded(timeinfo, forceRedraw);
}

void ClockWidget::updateIfNeeded(struct tm& timeinfo, bool forceRedraw) {
    char buffer[3];  // Enough for 2 digits + null terminator

    // Update hours if changed or forced
    if (forceRedraw || timeinfo.tm_hour != hours_.lastValue) {
        snprintf(buffer, sizeof(buffer), "%02d", timeinfo.tm_hour);
        drawTimePart(hours_.x, dimensions_.y, hours_.width, buffer);
        hours_.lastValue = timeinfo.tm_hour;
    }

    // Update minutes if changed or forced
    if (forceRedraw || timeinfo.tm_min != mins_.lastValue) {
        snprintf(buffer, sizeof(buffer), "%02d", timeinfo.tm_min);
        drawTimePart(mins_.x, dimensions_.y, mins_.width, buffer);
        mins_.lastValue = timeinfo.tm_min;
    }

    // Update seconds if changed or forced
    if (forceRedraw || timeinfo.tm_sec != secs_.lastValue) {
        snprintf(buffer, sizeof(buffer), "%02d", timeinfo.tm_sec);
        drawTimePart(secs_.x, dimensions_.y, secs_.width, buffer);
        secs_.lastValue = timeinfo.tm_sec;
    }

    lastUpdateTimeMs_ = millis();
}

void ClockWidget::drawTimePart(uint16_t x, uint16_t y, uint16_t width, const char* text) {
    lcd_->fillRect(x, y, width, dimensions_.height, bgColor_);
    lcd_->setTextColor(textColor_, bgColor_);
    lcd_->setTextSize(textSize_);
    lcd_->setTextDatum(CL_DATUM);
    lcd_->drawString(text, x, y + dimensions_.height / 2 - (textSize_ * 8) / 2);
}

bool ClockWidget::handleTouch(uint16_t x, uint16_t y) { return false; }