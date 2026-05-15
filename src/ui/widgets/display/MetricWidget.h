#pragma once

#include <cstring>
#include <memory>

#include "ui/widgets/base/Widget.h"

class MetricWidget : public Widget {
 public:
    // Format modes for integer-to-string conversion.
    // Replaces the heap-allocating std::function<String(int)> formatter.
    enum class ValueFormat : uint8_t {
        kDefault,   // "<value><unit>" (uses unit_ field, same as no formatter)
        kPercent,   // "<value>%"
        kRpm,       // "<value> RPM"
        kWatts,     // "<value>W"
        kCelsius,   // "<value>°C"
        kMB,        // "<value> MB"
    };

    MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs);

    void drawStatic() override;
    bool handleTouch(uint16_t x, uint16_t y) override;

    // Value configuration
    void setValue(int value);
    void setUnit(const char* unit);
    void setRange(int minValue, int maxValue);
    void setColorThresholds(float lowerThreshold, float upperThreshold);

    // Label configuration
    void setLabel(const char* label);
    void setLabelWidth(uint16_t width);

    // Display configuration
    void setTextAlignment(uint8_t alignment);  // TL_DATUM, TC_DATUM, TR_DATUM, etc.
    void setValueFormat(ValueFormat format);
    void setReverseThresholds(bool reverse = true);

    // Called by PcMetricsWidget's batch update: assumes NotoSans18 (metric font)
    // is already loaded by the caller.  Skips loadFont/unloadFont overhead.
    // Only valid when the background colour hasn't changed (value-only update).
    void drawValueWithLoadedFont();
    void setUseDimColors(bool useDim = false);

    void forceRefresh();

    // Getters
    int getValue() const { return value_; }
    int getMinValue() const { return minValue_; }
    int getMaxValue() const { return maxValue_; }
    const char* getUnit() const { return unit_; }
    float getLowerThreshold() const { return lowerThreshold_; }
    float getUpperThreshold() const { return upperThreshold_; }
    bool getUseDimColors() const { return useDimColors_; }

    // Builder pattern for fluent configuration
    class Builder {
     public:
        Builder(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
            : dims_(dims), updateIntervalMs_(updateIntervalMs) {}

        Builder& value(int value) {
            value_ = value;
            hasValue_ = true;
            return *this;
        }

        Builder& unit(const char* unit) {
            if (unit) {
                strncpy(unit_, unit, sizeof(unit_) - 1);
                unit_[sizeof(unit_) - 1] = '\0';
                hasUnit_ = true;
            }
            return *this;
        }

        Builder& range(int minVal, int maxVal) {
            minValue_ = minVal;
            maxValue_ = maxVal;
            hasRange_ = true;
            return *this;
        }

        Builder& colorThresholds(float lower, float upper) {
            lowerThreshold_ = lower;
            upperThreshold_ = upper;
            hasColorThresholds_ = true;
            return *this;
        }

        Builder& reverseThresholds(bool reverse = true) {
            reverseThresholds_ = reverse;
            hasReverseThresholds_ = true;
            return *this;
        }

        Builder& useDimColors(bool useDim = true) {
            useDimColors_ = useDim;
            hasUseDimColors_ = true;
            return *this;
        }

        Builder& label(const char* label) {
            if (label) {
                strncpy(label_, label, sizeof(label_) - 1);
                label_[sizeof(label_) - 1] = '\0';
                hasLabel_ = true;
            }
            return *this;
        }

        Builder& labelWidth(uint16_t width) {
            labelWidth_ = width;
            hasLabelWidth_ = true;
            return *this;
        }

        Builder& textAlignment(uint8_t alignment) {
            textAlignment_ = alignment;
            hasTextAlignment_ = true;
            return *this;
        }

        Builder& valueFormat(ValueFormat format) {
            valueFormat_ = format;
            hasValueFormat_ = true;
            return *this;
        }

        std::unique_ptr<MetricWidget> build() {
            auto widget = std::make_unique<MetricWidget>(dims_, updateIntervalMs_);

            // Apply configurations
            if (hasValue_)
                widget->setValue(value_);
            if (hasUnit_)
                widget->setUnit(unit_);
            if (hasRange_)
                widget->setRange(minValue_, maxValue_);
            if (hasColorThresholds_)
                widget->setColorThresholds(lowerThreshold_, upperThreshold_);
            if (hasReverseThresholds_)
                widget->setReverseThresholds(reverseThresholds_);
            if (hasUseDimColors_)
                widget->setUseDimColors(useDimColors_);
            if (hasLabel_)
                widget->setLabel(label_);
            if (hasLabelWidth_)
                widget->setLabelWidth(labelWidth_);
            if (hasTextAlignment_)
                widget->setTextAlignment(textAlignment_);
            if (hasValueFormat_)
                widget->setValueFormat(valueFormat_);

            return widget;
        }

     private:
        WidgetInterface::Dimensions dims_;
        uint32_t updateIntervalMs_;

        // Configuration parameters with flags
        int value_ = 0;
        char unit_[8] = "%";
        int minValue_ = 0;
        int maxValue_ = 100;
        float lowerThreshold_ = 50.0f;
        float upperThreshold_ = 90.0f;
        bool reverseThresholds_ = false;
        bool useDimColors_ = false;
        char label_[32] = "";
        uint16_t labelWidth_ = 0;
        uint8_t textAlignment_ = MC_DATUM;
        ValueFormat valueFormat_ = ValueFormat::kDefault;

        bool hasValue_ = false;
        bool hasUnit_ = false;
        bool hasRange_ = false;
        bool hasColorThresholds_ = false;
        bool hasReverseThresholds_ = false;
        bool hasUseDimColors_ = false;
        bool hasLabel_ = false;
        bool hasLabelWidth_ = false;
        bool hasTextAlignment_ = false;
        bool hasValueFormat_ = false;
    };

 protected:
    void onDraw(bool forceRedraw) override;

 private:
    // Configuration
    int value_ = 0;
    int minValue_ = 0;
    int maxValue_ = 100;
    char unit_[8] = "%";  // Stack-allocated buffer
    float lowerThreshold_ = 50.0f;
    float upperThreshold_ = 90.0f;
    bool reverseThresholds_ = false;
    bool useDimColors_ = false;
    char label_[32] = "";  // Stack-allocated buffer
    uint16_t labelWidth_ = 0;
    uint8_t textAlignment_ = MC_DATUM;
    ValueFormat valueFormat_ = ValueFormat::kDefault;

    // State
    bool hasLabel_ = false;
    int16_t valueX_ = 0;
    uint16_t valueWidth_ = 0;
    bool dimensionsDirty_ = true;
    int lastDrawnValue_ = -1;
    uint16_t lastBgColor_ = TFT_BLACK;
    bool valueAreaDirty_ = true;

    // Cached values for performance
    mutable char formattedValue_[24];  // Buffer for formatted value
    mutable bool formatCacheDirty_ = true;

    // Constants
    static constexpr uint16_t TEXT_MARGIN = 10;
    static constexpr uint16_t SEPARATOR_WIDTH = 1;
    static constexpr uint16_t BORDER_MARGIN = 1;

    // Rendering methods
    void renderValueArea();
    void renderValueTextOnly();
    void drawDebugPixel(int16_t startX, int16_t textY, int16_t width);

    // Helper methods
    uint16_t calculateBackgroundColor() const;
    void updateDimensionCache();
    const char* getFormattedValueText() const;
    void safeStringCopy(char* dest, const char* src, size_t destSize) const;
};