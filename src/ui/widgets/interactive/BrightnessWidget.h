#pragma once

#include "config/AppConfig.h"
#include "config/AppSettings.h"
#include "ui/core/DisplayManager.h"
#include "ui/widgets/base/Widget.h"

// Displays tappable brightness segments (one per AppConfig brightness level);
// the active level is highlighted.
//
// Layout (476 × 48 px example, 6 levels):
//
//   ┌──────────────────────────────────────────────────────┐
//   │  BRIGHTNESS                              (label)     │
//   │  [░1][░░2][▒▒3][▒▒4][▓▓5][██6]                        │
//   └──────────────────────────────────────────────────────┘
//
// Segment count is a compile-time AppConfig constant (it sizes the fixed
// segments_ array below); the level values themselves come from AppSettings
// (config_.uiBrightnessLevels) since they're data a user retunes per machine.
class BrightnessWidget : public Widget {
 public:
    static constexpr uint8_t kSegmentCount = AppConfig::internal::UiImpl::kBrightnessLevelCount;

    BrightnessWidget(const WidgetInterface::Dimensions& dims, DisplayManager& displayManager,
                     const AppSettings& config);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    struct Segment {
        uint16_t x;  // left pixel of this segment
        uint16_t width;
        uint8_t level;  // brightness value
        const char* label;
        uint16_t activeColor;
    };

    DisplayManager& displayManager_;
    const AppSettings& config_;
    uint8_t lastDrawnLevel_ = 0;

    Segment segments_[kSegmentCount];

    void buildSegments();
    void drawSegment(const Segment& seg, bool active);

    static constexpr uint8_t kLabelH = 13;  // px for "BRIGHTNESS" label row
    static constexpr uint8_t kGap = 3;      // px gap between segments
    static constexpr uint8_t kRadius = 4;   // corner radius
};
