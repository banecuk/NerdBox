#pragma once
#include <string>

#include "core/resources/FontRegistry.h"
#include "ui/widgets/base/Widget.h"

class ClockWidget : public Widget {
 public:
    ClockWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs = 1000,
                uint16_t textColor = TFT_LIGHTGREY, uint16_t bgColor = TFT_BLACK,
                const std::string& format = "%H:%M:%S");

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    void computeLayout();
    void updateIfNeeded(struct tm& timeinfo, bool forceRedraw);

    uint16_t textColor_;
    uint16_t bgColor_;

    // Font metrics — measured once after first loadFont call.
    uint16_t fontH_       = 0;  // font cap height in pixels
    uint16_t digitW_      = 0;  // width of one digit (monospaced — all digits equal)
    uint16_t colonW_      = 0;  // width of ':'
    bool     layoutReady_ = false;

    // Pixel positions of each field's left edge (right-aligned in widget).
    uint16_t xHours_  = 0;
    uint16_t xColon1_ = 0;
    uint16_t xMins_   = 0;
    uint16_t xColon2_ = 0;
    uint16_t xSecs_   = 0;
    uint16_t yText_   = 0;  // baseline y for MC_DATUM centering

    struct Field {
        int lastValue = -1;
    } hours_, mins_, secs_;
};
