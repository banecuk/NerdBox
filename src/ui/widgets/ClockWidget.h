#pragma once

#include <string>

#include "ui/widgets/Widget.h"

class ClockWidget : public Widget {
   public:
    ClockWidget(const Dimensions& dims,
                uint32_t updateIntervalMs = 1000,
                uint16_t textColor = TFT_LIGHTGREY,
                uint16_t bgColor = TFT_BLACK,
                uint8_t textSize = 3,
                const std::string& format = "%H:%M:%S");

    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;

   private:
    void drawTimePart(uint16_t x, uint16_t y, uint16_t width, const char* text);
    void updateIfNeeded(struct tm& timeinfo, bool forceRedraw);

    uint16_t textColor_;
    uint16_t bgColor_;
    uint8_t textSize_;
    std::string format_;

    // Time section dimensions
    struct TimeSection {
        uint16_t x;
        uint16_t width;
        int lastValue;
    } hours_, mins_, secs_;

    // Colon positions
    uint16_t colon1X_;
    uint16_t colon2X_;
    uint16_t colonY_;
    uint16_t colonWidth_ = 10; // Width reserved for each colon
};