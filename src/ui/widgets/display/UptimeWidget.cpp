#include "UptimeWidget.h"

#include <cstring>

UptimeWidget::UptimeWidget(const WidgetInterface::Dimensions& dims,
                           ApplicationMetrics& systemMetrics,
                           uint16_t textColor,
                           uint16_t bgColor)
    : Widget(dims, 1000),       // re-evaluate every second
      systemMetrics_(systemMetrics),
      textColor_(textColor),
      bgColor_(bgColor) {}

void UptimeWidget::drawStatic() {
    if (!isInitialized_ || !getLcd())
        return;

    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    // Small grey label, same style as IpAddressWidget
    lcd->setTextSize(1);
    lcd->setTextColor(TFT_DARKGREY, bgColor_);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString("UPTIME", dimensions_.x, dimensions_.y + 2);

    isStaticDrawn_ = true;
    lastRendered_[0] = '\0';  // force a full value draw on the next onDraw()
    clearDirty();
}

void UptimeWidget::onDraw(bool forceRedraw) {
    if (!getLcd())
        return;

    char current[9];
    systemMetrics_.getFormattedUptime(current, sizeof(current));

    // Skip repaint entirely when nothing has changed (and no force).
    if (!forceRedraw && strncmp(current, lastRendered_, sizeof(current)) == 0) {
        clearDirty();
        return;
    }

    LGFX* lcd = getLcd();

    // Paint each field in-place using a solid background colour — identical to
    // ClockWidget's drawTimePart().  This overwrites the previous digits without
    // a prior fillRect(), so there is never a blank frame between erase and draw
    // and the flash is eliminated.
    const uint16_t valueY  = dimensions_.y + 14;
    const uint16_t valueH  = dimensions_.height - 14;
    const uint8_t  charW   = 12;   // size-2 font is 6px * 2 = 12px per char
    const uint16_t fieldW  = charW * 2;  // two digits per field

    lcd->setTextSize(2);
    lcd->setTextDatum(TL_DATUM);
    lcd->setTextColor(textColor_, bgColor_);  // bgColor_ as the text background kills the flash

    // Only redraw the fields whose digit(s) actually changed.
    // HH changes at most once an hour; MM once a minute; SS every second.
    char prev[9];
    strncpy(prev, lastRendered_, sizeof(prev));

    // Helper: overwrite one 2-digit field if it differs (or force).
    // x is the pixel offset from dimensions_.x.
    auto drawField = [&](uint16_t x, const char* cur2, const char* old2) {
        if (forceRedraw || strncmp(cur2, old2, 2) != 0) {
            char buf[3] = {cur2[0], cur2[1], '\0'};
            lcd->drawString(buf, dimensions_.x + x, valueY + 2);
        }
    };

    // Separator colons — draw once on force, never need re-clearing.
    if (forceRedraw || lastRendered_[0] == '\0') {
        lcd->setTextColor(TFT_DARKGREY, bgColor_);
        lcd->drawString(":", dimensions_.x + fieldW,           valueY + 2);
        lcd->drawString(":", dimensions_.x + fieldW * 2 + charW, valueY + 2);
        lcd->setTextColor(textColor_, bgColor_);
    }

    // Hours / minutes / seconds — each field is 2 chars wide.
    drawField(0,                    current,     prev);       // HH
    drawField(fieldW + charW,       current + 3, prev + 3);  // MM  (skip "HH:")
    drawField(fieldW * 2 + charW * 2, current + 6, prev + 6);  // SS  (skip "HH:MM:")

    strncpy(lastRendered_, current, sizeof(lastRendered_));
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

bool UptimeWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
