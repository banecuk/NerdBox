#include "BootScreen.h"

#include "core/resources/FontRegistry.h"

BootScreen::BootScreen(ScreenLogQueue& screenLogQueue, LGFX* lcd)
    : screenLogQueue_(screenLogQueue), lcd_(lcd) {}

void BootScreen::initialize() {}

void BootScreen::onEnter() {
    if (!lcd_)
        return;

    lcd_->fillScreen(TFT_BLACK);

    // Title — NotoSans18, shadow offset by 1px for depth
    Fonts::loadMetric(lcd_);
    lcd_->setTextDatum(TL_DATUM);
    lcd_->setTextColor(TFT_DARKGRAY, TFT_BLACK);
    lcd_->drawString("NerdBox", 1, 1);
    lcd_->setTextColor(TFT_DARKCYAN, TFT_BLACK);
    lcd_->drawString("NerdBox", 0, 0);
    Fonts::unload(lcd_);

    // Measure label font height once so log lines are spaced correctly.
    Fonts::loadLabel(lcd_);
    lineHeight_ = static_cast<uint16_t>(lcd_->fontHeight()) + 2;
    Fonts::unload(lcd_);

    // First log line starts below the title (NotoSans18 cap height ~22px + margin).
    lineY_ = kLogAreaY;

    // Confine scroll() (called from draw() once the log area fills up) to
    // the log area only, so it never disturbs the title above it.
    lcd_->setClipRect(0, kLogAreaY, lcd_->width(), lcd_->height() - kLogAreaY);
}

void BootScreen::onExit() {
    if (lcd_)
        lcd_->clearClipRect();
}

void BootScreen::draw() {
    if (!lcd_)
        return;

    char message[200];
    while (screenLogQueue_.popScreenMessage(message, sizeof(message))) {
        // Once the log area is full, scroll its contents up by one line
        // instead of drawing past the bottom of the screen — otherwise later
        // boot messages land off-screen and are simply never seen.
        if (lineY_ + lineHeight_ > lcd_->height()) {
            lcd_->scroll(0, -static_cast<int_fast16_t>(lineHeight_));
            lineY_ -= lineHeight_;
        }

        Fonts::loadLabel(lcd_);
        lcd_->setTextColor(TFT_WHITE, TFT_BLACK);
        lcd_->setTextDatum(TL_DATUM);
        lcd_->drawString(message, 0, lineY_);
        Fonts::unload(lcd_);

        lineY_ += lineHeight_;
    }
}
