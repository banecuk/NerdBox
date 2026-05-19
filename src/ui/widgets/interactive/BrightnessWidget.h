#pragma once

#include "config/AppConfig.h"
#include "ui/core/DisplayManager.h"
#include "ui/widgets/base/Widget.h"

// Displays five tappable brightness segments; the active level is highlighted.
//
// Layout (476 × 48 px example):
//
//   ┌──────────────────────────────────────────────────────┐
//   │  BRIGHTNESS                              (label)     │
//   │  [ ░ 1 ][ ░░ 2 ][ ▒▒ 3 ][ ▓▓ 4 ][ ██ 5 ]           │
//   └──────────────────────────────────────────────────────┘
//
// Level values and count come from AppConfig::internal::UiImpl::kBrightnessLevels
// so adding a new step requires only a config change.
class BrightnessWidget : public Widget {
 public:
    static constexpr uint8_t kSegmentCount =
        AppConfig::internal::UiImpl::kBrightnessLevelCount;

    BrightnessWidget(const WidgetInterface::Dimensions& dims, DisplayManager& displayManager);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    struct Segment {
        uint16_t    x;            // left pixel of this segment
        uint16_t    width;
        uint8_t     level;        // brightness value
        const char* label;
        uint16_t    activeColor;
    };

    DisplayManager& displayManager_;
    uint8_t         lastDrawnLevel_ = 0;

    Segment segments_[kSegmentCount];

    void buildSegments();
    void drawSegment(const Segment& seg, bool active);

    static constexpr uint8_t kLabelH = 13;  // px for "BRIGHTNESS" label row
    static constexpr uint8_t kGap    = 3;   // px gap between segments
    static constexpr uint8_t kRadius = 4;   // corner radius
};
