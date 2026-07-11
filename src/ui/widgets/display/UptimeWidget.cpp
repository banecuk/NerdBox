#include "UptimeWidget.h"

#include <cstring>

UptimeWidget::UptimeWidget(const WidgetInterface::Dimensions& dims,
                           ApplicationMetrics& systemMetrics,
                           uint16_t textColor, uint16_t bgColor)
    : Widget(dims, 1000),
      systemMetrics_(systemMetrics),
      textColor_(textColor),
      bgColor_(bgColor) {}

// ---------------------------------------------------------------------------
// Layout — deferred until getLcd() is valid.
// ---------------------------------------------------------------------------
void UptimeWidget::computeLayout() {
    LGFX* lcd = getLcd();
    if (!lcd) return;

    // Measure label height so we know where the value row starts.
    Fonts::loadLabel(lcd);
    const uint16_t labelH = static_cast<uint16_t>(lcd->fontHeight());
    Fonts::unload(lcd);

    // Measure value-font glyph widths for field-level redraw.
    Fonts::loadValue(lcd);
    digitW_ = static_cast<uint16_t>(lcd->textWidth("00"));
    colonW_ = static_cast<uint16_t>(lcd->textWidth(":"));
    const uint16_t valH = static_cast<uint16_t>(lcd->fontHeight());
    Fonts::unload(lcd);

    // valueY_ is the vertical midpoint of the value row.
    // All draw calls use ML_DATUM so colons and digit fields share the same
    // baseline — placing the glyph midpoint at valueY_.
    const uint16_t pad      = 2;
    const uint16_t belowLabel = dimensions_.height - labelH - pad;
    valueY_ = dimensions_.y + labelH + pad + belowLabel / 2;

    // Left-aligned field positions.
    xHH_     = dimensions_.x;
    xColon1_ = xHH_     + digitW_;
    xMM_     = xColon1_ + colonW_;
    xColon2_ = xMM_     + digitW_;
    xSS_     = xColon2_ + colonW_;

    layoutReady_ = true;
}

void UptimeWidget::onDrawStatic() {
    LGFX* lcd = getLcd();
    lcd->fillRect(dimensions_.x, dimensions_.y, dimensions_.width, dimensions_.height, bgColor_);

    if (!layoutReady_)
        computeLayout();

    Fonts::loadLabel(lcd);
    lcd->setTextColor(TFT_DARKGREY, bgColor_);
    lcd->setTextDatum(TL_DATUM);
    lcd->drawString("UPTIME", dimensions_.x, dimensions_.y + 2);
    Fonts::unload(lcd);

    // Draw static colons in value font at the vertical midpoint of the value
    // row so they align with digit cap height — ML_DATUM places the glyph
    // midpoint at valueY_, matching how onDraw() positions each digit field.
    Fonts::loadValue(lcd);
    lcd->setTextColor(TFT_DARKGREY, bgColor_);
    lcd->setTextDatum(ML_DATUM);
    lcd->drawString(":", xColon1_, valueY_);
    lcd->drawString(":", xColon2_, valueY_);
    Fonts::unload(lcd);

    lastRendered_[0] = '\0';
}

void UptimeWidget::onDraw(bool forceRedraw) {
    if (!getLcd() || !layoutReady_)
        return;

    char current[9];
    systemMetrics_.getFormattedUptime(current, sizeof(current));

    if (!forceRedraw && strncmp(current, lastRendered_, sizeof(current)) == 0) {
        clearDirty();
        return;
    }

    LGFX* lcd = getLcd();
    char prev[9];
    strncpy(prev, lastRendered_, sizeof(prev));

    // Load value font once for all three fields.
    // ML_DATUM — left-edge x, vertical midpoint y — matches the colons drawn
    // in drawStatic(), so all six glyphs sit on the same optical baseline.
    Fonts::loadValue(lcd);
    lcd->setTextDatum(ML_DATUM);
    lcd->setTextColor(textColor_, bgColor_);  // bg param = per-glyph fill, no flicker

    auto drawField = [&](uint16_t x, const char* cur2, const char* old2) {
        if (forceRedraw || strncmp(cur2, old2, 2) != 0) {
            char buf[3] = {cur2[0], cur2[1], '\0'};
            lcd->drawString(buf, x, valueY_);
        }
    };

    drawField(xHH_, current,     prev);      // HH
    drawField(xMM_, current + 3, prev + 3);  // MM
    drawField(xSS_, current + 6, prev + 6);  // SS

    Fonts::unload(lcd);

    strncpy(lastRendered_, current, sizeof(lastRendered_));
    lastUpdateTimeMs_ = millis();
    clearDirty();
}

bool UptimeWidget::handleTouch(uint16_t x, uint16_t y) {
    return false;
}
