#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include <string>

#include "ui/widgets/Widget.h"

class ClockWidget : public Widget {
   public:
    ClockWidget(const Dimensions& dims,              // Position and size
                uint32_t updateIntervalMs = 1000,    // Default update check every second
                uint16_t textColor = TFT_LIGHTGREY,  // Default text color
                uint16_t bgColor = TFT_BLACK,        // Default background color
                uint8_t textSize = 3,                // Default text size
                const std::string& format = "%H:%M:%S"  // Default time format
    );

    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;

   private:
    uint16_t textColor_;
    uint16_t bgColor_;
    uint8_t textSize_;
    std::string format_;

    // State to prevent unnecessary redraws
    int lastDisplayedSecond_ = -1;
};

#endif  // CLOCKWIDGET_H