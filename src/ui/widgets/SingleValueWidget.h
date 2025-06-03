#pragma once

#include "ui/widgets/Widget.h"

class SingleValueWidget : public Widget {
public:
    SingleValueWidget(const Dimensions& dims, uint32_t updateIntervalMs);
    
    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    bool handleTouch(uint16_t x, uint16_t y) override;
    
    void setValue(int value);
    void setUnit(const String& unit);
    void setRange(int minValue, int maxValue);
    void setColorThresholds(float greenThreshold, float yellowThreshold, float redThreshold);

private:
    int value_ = 0;
    int minValue_ = 0;
    int maxValue_ = 100;
    String unit_ = "%";
    
    float greenThreshold_ = 0.5f;
    float yellowThreshold_ = 0.75f;
    float redThreshold_ = 1.0f;
    
    uint16_t getBackgroundColor() const;
    uint16_t getTextColor(uint16_t bgColor) const;
    void drawValue();
};