#pragma once

#include <cstdio>

#include <LovyanGFX.hpp>

// Shared field-diff mechanics for widgets that show a live HH:MM:SS clock
// with per-field redraw (only the digit pair that actually changed since the
// last draw gets repainted). Layout (x positions, font choice, colors,
// colon drawing) stays widget-specific — this only owns "did a field
// change" and "paint a 2-digit field at ML_DATUM".
class HmsFieldRenderer {
 public:
    struct Positions {
        uint16_t xHours = 0;
        uint16_t xMins = 0;
        uint16_t xSecs = 0;
        uint16_t y = 0;
    };

    void setPositions(const Positions& pos) { pos_ = pos; }

    // Forces the next draw() call to repaint every field.
    void resetFields() { hours_ = mins_ = secs_ = -1; }

    bool hasChange(int hours, int mins, int secs, bool forceRedraw) const {
        return forceRedraw || hours != hours_ || mins != mins_ || secs != secs_;
    }

    // Caller must already have loaded the font and set the text color;
    // this sets ML_DATUM itself and only touches fields that changed.
    void draw(LGFX* lcd, int hours, int mins, int secs, bool forceRedraw) {
        lcd->setTextDatum(ML_DATUM);
        char buf[3];
        if (forceRedraw || hours != hours_) {
            snprintf(buf, sizeof(buf), "%02d", hours);
            lcd->drawString(buf, pos_.xHours, pos_.y);
            hours_ = hours;
        }
        if (forceRedraw || mins != mins_) {
            snprintf(buf, sizeof(buf), "%02d", mins);
            lcd->drawString(buf, pos_.xMins, pos_.y);
            mins_ = mins;
        }
        if (forceRedraw || secs != secs_) {
            snprintf(buf, sizeof(buf), "%02d", secs);
            lcd->drawString(buf, pos_.xSecs, pos_.y);
            secs_ = secs;
        }
    }

 private:
    Positions pos_;
    int hours_ = -1;
    int mins_ = -1;
    int secs_ = -1;
};
