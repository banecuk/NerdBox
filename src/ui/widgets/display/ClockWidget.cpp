#include "ClockWidget.h"

#include <cstdio>

#include <time.h>

ClockWidget::ClockWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                         uint16_t textColor, uint16_t bgColor, const std::string& format)
    : Widget(dims, updateIntervalMs), textColor_(textColor), bgColor_(bgColor), format_(format) {}

// ---------------------------------------------------------------------------
// Layout — deferred until after initialize() so getLcd() is valid.
// Called once, on the first drawStatic().
// ---------------------------------------------------------------------------
void ClockWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Measure glyph dimensions with the actual font so positions are exact.
    lcd->loadFont(NotoSansMono24);
    lcd->setTextSize(1);

    // fontHeight() gives the full line height; we want the cap height for
    // vertical centering.  Use the height of "0" as a proxy — it equals the
    // cap height for a monospaced digit font.
    fontH_ = static_cast<uint16_t>(lcd->fontHeight());
    digitW_ = static_cast<uint16_t>(lcd->textWidth("00"));  // 2-digit field
    colonW_ = static_cast<uint16_t>(lcd->textWidth(":"));

    lcd->unloadFont();

    // Full string width: HH : MM : SS
    const uint16_t totalW = digitW_ * 3 + colonW_ * 2;

    // Right-align inside the widget — start x is right edge minus total width.
    const uint16_t startX = dimensions_.x + dimensions_.width - totalW;

    xHours_ = startX;
    xColon1_ = xHours_ + digitW_;
    xMins_ = xColon1_ + colonW_;
    xColon2_ = xMins_ + digitW_;
    xSecs_ = xColon2_ + colonW_;
    xColon1_ = xColon1_ + digitW_ / 4;
    xColon2_ = xColon2_ + digitW_ / 4;

    // Vertical center — MC_DATUM draws from the midpoint of the glyph.
    yText_ = dimensions_.y + dimensions_.height / 2;

    layoutReady_ = true;
}

// ---------------------------------------------------------------------------
// drawStatic — clear background, measure font, paint static colons.
// ---------------------------------------------------------------------------
void ClockWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    LGFX* lcd = getLcd();

    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    if (!layoutReady_)
        computeLayout();

    // Colons are static chrome — drawn here, never redrawn.
    lcd->loadFont(NotoSansMono24);
    lcd->setTextSize(1);
    lcd->setTextColor(textColor_, bgColor_);
    lcd->setTextDatum(MC_DATUM);
    lcd->drawString(":", xColon1_, yText_);
    lcd->drawString(":", xColon2_, yText_);
    lcd->unloadFont();

    // Force full digit repaint on next onDraw.
    hours_.lastValue = -1;
    mins_.lastValue = -1;
    secs_.lastValue = -1;

    isStaticDrawn_ = true;
    clearDirty();
}

// ---------------------------------------------------------------------------
// onDraw — called every second (or on force).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// drawField — overwrites a 2-digit area in-place.
//
// The key anti-flicker technique: setTextColor(fg, bg) tells LovyanGFX to
// fill each glyph's bounding box with bg before drawing the foreground pixels.
// This is a single GPU operation per glyph — there is no separate erase pass
// and therefore no blank frame between the old and new digit.
// fillRect() before drawString() produces two display operations and is the
// direct cause of flicker; we never use it here.
// ---------------------------------------------------------------------------
void ClockWidget::drawField(const char* text, uint16_t x) {
    LGFX* lcd = getLcd();
    lcd->loadFont(NotoSansMono24);
    lcd->setTextSize(1);
    lcd->setTextColor(textColor_, bgColor_);  // bg param = per-glyph background fill
    lcd->setTextDatum(ML_DATUM);              // left-align within the field
    lcd->drawString(text, x, yText_);
    lcd->unloadFont();
}

bool ClockWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
