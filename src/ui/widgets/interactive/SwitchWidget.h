#pragma once

#include <functional>
#include <string>

#include "ui/core/Colors.h"
#include "ui/widgets/base/Widget.h"

// Tappable ON/OFF switch with a label, for boolean settings (e.g. "dim at
// night"). State is not owned by the widget — getState()/setState() read and
// write through to whatever backing store the caller supplies, so the widget
// always reflects the true current value even if it changes elsewhere.
class SwitchWidget : public Widget {
 public:
    using GetStateFn = std::function<bool()>;
    using SetStateFn = std::function<void(bool)>;

    SwitchWidget(const WidgetInterface::Dimensions& dims, std::string label,
                 GetStateFn getState, SetStateFn setState);

    bool handleTouch(uint16_t x, uint16_t y) override;

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    void drawTrack(bool on);

    std::string label_;
    GetStateFn getState_;
    SetStateFn setState_;

    bool lastDrawnState_ = false;
    bool hasDrawnOnce_ = false;

    static constexpr uint8_t kLabelH = 13;   // px for the label row
    static constexpr uint8_t kGap = 3;       // px gap between label and track
    static constexpr uint8_t kRadius = 4;    // corner radius
    static constexpr uint16_t kOnColor = 0x07E0;    // green
    static constexpr uint16_t kOffColor = Colors::kHairline;
};
