#pragma once

#include "ui/widgets/Widget.h"

class SingleValueWidget : public Widget {
   public:
    SingleValueWidget(DisplayContext& context, const Dimensions& dims,
                      uint32_t updateIntervalMs);

    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;

    void setValue(int value);
    void setUnit(const String& unit);
    void setRange(int minValue, int maxValue);
    void setColorThresholds(float greenThreshold, float yellowThreshold,
                            float redThreshold);

    void setLabel(const String& label);
    void setLabelWidth(uint16_t width);

   private:
    int value_ = 0;
    int minValue_ = 0;
    int maxValue_ = 100;
    String unit_ = "%";

    float greenThreshold_ = 0.5f;
    float yellowThreshold_ = 0.75f;
    float redThreshold_ = 1.0f;

    String label_;
    uint16_t labelWidth_ = 0;
    bool hasLabel_ = false;

    // Cached values for optimization
    int16_t valueX_ = 0;
    uint16_t valueWidth_ = 0;
    uint8_t optimalTextSize_ = 1;
    bool dimensionsDirty_ = true;
    bool textSizeDirty_ = true;

    uint16_t getBackgroundColor() const;
    void drawValue();
    void updateDimensions();
    void updateTextSize();

    DisplayContext& context_;
};