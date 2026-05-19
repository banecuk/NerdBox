#include "ClockWidget.h"

#include <cstdio>
#include <time.h>

ClockWidget::ClockWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         uint16_t textColor, uint16_t bgColor, const std::string& format)
    : Widget(dims, updateIntervalMs),
      textColor_(textColor),
      bgColor_(bgColor),
      format_(format) {}

void ClockWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    Fonts::loadMono(lcd);
    

    fontH_  = static_cast<uint16_t>(lcd->fontHeight());
    digitW_ = static_cast<uint16_t>(lcd->textWidth("00"));
    colonW_ = static_cast<uint16_t>(lcd->textWidth(":"));

    Fonts::unload(lcd);

    const uint16_t totalW = digitW_ * 3 + colonW_ * 2;
    const uint16_t startX = dimensions_.x + dimensions_.width - totalW;

    xHours_  = startX;
    xColon1_ = xHours_  + digitW_;
    xMins_   = xColon1_ + colonW_;
    xColon2_ = xMins_   + digitW_;
    xSecs_   = xColon2_ + colonW_;

    // User tweak: nudge colons slightly right for optical spacing.
    xColon1_ = xColon1_ + digitW_ / 4;
    xColon2_ = xColon2_ + digitW_ / 4;

    yText_ = dimensions_.y + dimensions_.height / 2;

    layoutReady_ = true;
}

void ClockWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    if (!layoutReady_)
        computeLayout();

    Fonts::loadMono(lcd);
    
    lcd->setTextColor(textColor_, bgColor_);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(":", xColon1_, yText_);
    lcd->drawString(":", xColon2_, yText_);
    Fonts::unload(lcd);

    hours_.lastValue = -1;
    mins_.lastValue  = -1;
    secs_.lastValue  = -1;

    isStaticDrawn_ = true;
    clearDirty();
}

void ClockWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !layoutReady_)
        return;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5))
        return;

    updateIfNeeded(timeinfo, forceRedraw);
}

void ClockWidget::updateIfNeeded(struct tm& timeinfo, bool forceRedraw) {
    bool changed = false;

    if (forceRedraw || timeinfo.tm_hour != hours_.lastValue) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02d", timeinfo.tm_hour);
        drawField(buf, xHours_);
        hours_.lastValue = timeinfo.tm_hour;
        changed = true;
    }
    if (forceRedraw || timeinfo.tm_min != mins_.lastValue) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02d", timeinfo.tm_min);
        drawField(buf, xMins_);
        mins_.lastValue = timeinfo.tm_min;
        changed = true;
    }
    if (forceRedraw || timeinfo.tm_sec != secs_.lastValue) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02d", timeinfo.tm_sec);
        drawField(buf, xSecs_);
        secs_.lastValue = timeinfo.tm_sec;
        changed = true;
    }

    if (changed) {
        lastUpdateTimeMs_ = millis();
        clearDirty();
    }
}

void ClockWidget::drawField(const char* text, uint16_t x) {
    LGFX* lcd = getLcd();
    Fonts::loadMono(lcd);
    
    lcd->setTextColor(textColor_, bgColor_);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(text, x, yText_);
    Fonts::unload(lcd);
}

bool ClockWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
