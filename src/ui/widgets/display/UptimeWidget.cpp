#include "UptimeWidget.h"

#include <cstdlib>
#include <cstring>

#include "ui/widgets/base/WidgetPainter.h"

UptimeWidget::UptimeWidget(const WidgetInterface::Dimensions& dims,
                           ApplicationMetrics& systemMetrics, uint16_t textColor, uint16_t bgColor)
    : Widget(dims, 1000), systemMetrics_(systemMetrics), textColor_(textColor), bgColor_(bgColor) {}

// ---------------------------------------------------------------------------
// Layout — deferred until getLcd() is valid.
// ---------------------------------------------------------------------------
void UptimeWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd)
        return;

    // Measure label height so we know where the value row starts.
    Fonts::loadLabel(lcd);
    const uint16_t labelH = static_cast<uint16_t>(lcd->fontHeight());
    Fonts::unload(lcd);

    // Measure value-font glyph widths for field-level redraw.
    Fonts::loadValue(lcd);
    digitW_ = static_cast<uint16_t>(lcd->textWidth("00"));
    colonW_ = static_cast<uint16_t>(lcd->textWidth(":"));
    Fonts::unload(lcd);

    // valueY_ is the vertical midpoint of the value row.
    // All draw calls use ML_DATUM so colons and digit fields share the same
    // baseline — placing the glyph midpoint at valueY_.
    const uint16_t pad = 2;
    const uint16_t belowLabel = dimensions_.height - labelH - pad;
    valueY_ = dimensions_.y + labelH + pad + belowLabel / 2;

    // Left-aligned field positions.
    xHH_ = dimensions_.x;
    xColon1_ = xHH_ + digitW_;
    const uint16_t xMM = xColon1_ + colonW_;
    xColon2_ = xMM + digitW_;
    const uint16_t xSS = xColon2_ + colonW_;

    fieldRenderer_.setPositions({xHH_, xMM, xSS, valueY_});

    layoutReady_ = true;
}

void UptimeWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    if (!layoutReady_)
        computeLayout();

    WidgetPainter::drawCaptionLabel(lcd, dimensions_.x, dimensions_.y, "UPTIME", bgColor_);

    // Draw static colons in value font at the vertical midpoint of the value
    // row so they align with digit cap height — ML_DATUM places the glyph
    // midpoint at valueY_, matching how onDraw() positions each digit field.
    // Skipped in day mode ("Dd HH:MM"), which draws itself as one string.
    if (!dayMode_) {
        Fonts::loadValue(lcd);
        lcd->setTextColor(TFT_DARKGREY, bgColor_);
        lcd->setTextDatum(ML_DATUM);
        lcd->drawString(":", xColon1_, valueY_);
        lcd->drawString(":", xColon2_, valueY_);
        Fonts::unload(lcd);
    }

    lastRendered_[0] = '\0';
    fieldRenderer_.resetFields();
}

void UptimeWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !layoutReady_)
        return;

    char current[16];
    systemMetrics_.getFormattedUptime(current, sizeof(current));
    const bool isDayMode = strchr(current, 'd') != nullptr;

    if (!forceRedraw && isDayMode == dayMode_ &&
        strncmp(current, lastRendered_, sizeof(current)) == 0) {
        clearDirty();
        return;
    }

    LGFX* lcd = getLcd();

    // Mode just changed (e.g. crossed 99h uptime) — the fixed-offset field
    // layout below no longer matches the string shape, so repaint the whole
    // widget (colons included) from scratch.
    if (isDayMode != dayMode_) {
        dayMode_ = isDayMode;
        onDrawStatic();
    }

    if (isDayMode) {
        // "Dd HH:MM" doesn't fit the fixed 2-digit-field layout — just redraw
        // the whole string when it changes.
        lcd->fillRect(dimensions_.x, valueY_ - digitW_ / 2, dimensions_.width, digitW_ + 2,
                      bgColor_);
        Fonts::loadValue(lcd);
        lcd->setTextDatum(ML_DATUM);
        lcd->setTextColor(textColor_, bgColor_);
        lcd->drawString(current, xHH_, valueY_);
        Fonts::unload(lcd);
    } else {
        // Load value font once for all three fields.
        // ML_DATUM — left-edge x, vertical midpoint y — matches the colons drawn
        // in drawStatic(), so all six glyphs sit on the same optical baseline.
        Fonts::loadValue(lcd);
        lcd->setTextColor(textColor_, bgColor_);  // bg param = per-glyph fill, no flicker
        fieldRenderer_.draw(lcd, atoi(current), atoi(current + 3), atoi(current + 6), forceRedraw);
        Fonts::unload(lcd);
    }

    strncpy(lastRendered_, current, sizeof(lastRendered_));
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

bool UptimeWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
