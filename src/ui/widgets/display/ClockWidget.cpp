#include "ClockWidget.h"

#include <cstdio>

#include <time.h>

ClockWidget::ClockWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         uint16_t textColor, uint16_t bgColor, const std::string& format,
                         EventType action, ActionCallback callback)
    : Widget(dims, updateIntervalMs),
      textColor_(textColor),
      bgColor_(bgColor),
      action_(action),
      callback_(std::move(callback)) {}

void ClockWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    Fonts::loadMono(lcd);

    fontH_ = static_cast<uint16_t>(lcd->fontHeight());
    digitW_ = static_cast<uint16_t>(lcd->textWidth("00"));
    colonW_ = static_cast<uint16_t>(lcd->textWidth(":"));

    Fonts::unload(lcd);

    const uint16_t totalW = digitW_ * 3 + colonW_ * 2;
    const uint16_t startX = dimensions_.x + dimensions_.width - totalW;

    xHours_ = startX;
    xColon1_ = xHours_ + digitW_;
    xMins_ = xColon1_ + colonW_;
    xColon2_ = xMins_ + digitW_;
    xSecs_ = xColon2_ + colonW_;

    // User tweak: nudge colons slightly right for optical spacing.
    xColon1_ = xColon1_ + digitW_ / 4;
    xColon2_ = xColon2_ + digitW_ / 4;

    yText_ = dimensions_.y + dimensions_.height / 2;

    fieldRenderer_.setPositions({xHours_, xMins_, xSecs_, yText_});

    layoutReady_ = true;
}

void ClockWidget::onDrawStatic() {
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

    fieldRenderer_.resetFields();
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
    if (!fieldRenderer_.hasChange(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, forceRedraw))
        return;

    // Load the font once for every field that changed this tick instead of
    // once per field — at steady state (seconds ticking) this was one
    // heap alloc/free every second; on minute/hour rollovers, up to three.
    LGFX* lcd = getLcd();
    Fonts::loadMono(lcd);
    lcd->setTextColor(textColor_, bgColor_);
    fieldRenderer_.draw(lcd, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, forceRedraw);
    Fonts::unload(lcd);

    lastUpdateTimeMs_ = millis();
    clearDirty();
}

bool ClockWidget::handleTouch(uint16_t /*x*/, uint16_t /*y*/) {
    if (!callback_)
        return false;
    callback_(action_);
    return true;
}
