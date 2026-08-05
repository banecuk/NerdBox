#pragma once

#include <cstring>
#include <memory>

#include "ui/widgets/base/Widget.h"

class MetricWidget : public Widget {
 public:
    // Format modes — select which unit suffix is drawn (smaller font, same
    // colour as the value) next to it. See getUnitText() in the .cpp.
    enum class ValueFormat : uint8_t {
        kDefault,   // unit_ field (same as no formatter)
        kPercent,   // "%"
        kRpm,       // "RPM"
        kWatts,     // "W"
        kCelsius,   // "°C"
        kMB,        // "MB"
    };

    MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs);

    bool handleTouch(uint16_t x, uint16_t y) override;

    // Value configuration
    void setValue(int value);
    void setUnit(const char* unit);
    void setRange(int minValue, int maxValue);
    void setColorThresholds(float lowerThreshold, float upperThreshold);

    // Label configuration
    void setLabel(const char* label);
    void setLabelWidth(uint16_t width);
    void setVerticalLabel(bool vertical = true);  // stack label chars top-to-bottom — for tall tiles

    // Display configuration
    void setTextAlignment(uint8_t alignment);  // TL_DATUM, TC_DATUM, TR_DATUM, etc.
    void setValueFormat(ValueFormat format);
    void setReverseThresholds(bool reverse = true);
    void setUseSmallFont(bool small = true);  // Use 15pt instead of 18pt — for narrow tiles
    void setLabelColor(uint16_t color);
    void setUseGpuColors(bool use = true);
    void setUseRamColors(bool use = true);

    // Called by PcMetricsWidget's batch update: assumes NotoSans18 (metric font)
    // is already loaded by the caller.  Skips loadFont/unloadFont overhead.
    // Only valid when the background colour hasn't changed (value-only update).
    void drawValueWithLoadedFont();

    // Paired with drawValueWithLoadedFont() — call after it, with the label
    // font (NotoSansDisplay12) loaded by the caller, to draw the unit
    // suffix. Only redraws when the preceding drawValueWithLoadedFont() call
    // actually moved or recoloured it; a no-op otherwise.
    void drawUnitWithLoadedFont();

    void setUseDimColors(bool useDim = false);

    void forceRefresh();

    // Sets the inset of the coloured value area from the widget's edges.
    // A zero margin makes the fill run flush to the widget bounds (used by
    // DiskBandWidget so its metric tiles sit immediately against the activity
    // lines). Defaults to BORDER_MARGIN (1px).
    void setBorderMargin(uint16_t margin);

    // Getters
    int getValue() const { return value_; }
    int getMinValue() const { return minValue_; }
    int getMaxValue() const { return maxValue_; }
    const char* getUnit() const { return unit_; }
    float getLowerThreshold() const { return lowerThreshold_; }
    float getUpperThreshold() const { return upperThreshold_; }
    bool getUseDimColors() const { return useDimColors_; }
    bool isSmallFont() const { return useSmallFont_; }

    // Builder pattern for fluent configuration. Each setter's default here
    // matches the widget's own default member initializer, so build() can
    // apply every field unconditionally instead of tracking a hasX_ flag
    // per field.
    class Builder {
     public:
        Builder(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs)
            : dims_(dims), updateIntervalMs_(updateIntervalMs) {}

        Builder& value(int value) {
            value_ = value;
            return *this;
        }

        Builder& unit(const char* unit) {
            if (unit) {
                strncpy(unit_, unit, sizeof(unit_) - 1);
                unit_[sizeof(unit_) - 1] = '\0';
            }
            return *this;
        }

        Builder& range(int minVal, int maxVal) {
            minValue_ = minVal;
            maxValue_ = maxVal;
            return *this;
        }

        Builder& colorThresholds(float lower, float upper) {
            lowerThreshold_ = lower;
            upperThreshold_ = upper;
            return *this;
        }

        Builder& reverseThresholds(bool reverse = true) {
            reverseThresholds_ = reverse;
            return *this;
        }

        Builder& smallFont(bool small = true) {
            useSmallFont_ = small;
            return *this;
        }

        Builder& useDimColors(bool useDim = true) {
            useDimColors_ = useDim;
            return *this;
        }

        Builder& labelColor(uint16_t color) {
            labelColor_ = color;
            return *this;
        }

        Builder& useGpuColors(bool use = true) {
            useGpuColors_ = use;
            return *this;
        }

        Builder& useRamColors(bool use = true) {
            useRamColors_ = use;
            return *this;
        }

        Builder& label(const char* label) {
            if (label) {
                strncpy(label_, label, sizeof(label_) - 1);
                label_[sizeof(label_) - 1] = '\0';
            }
            return *this;
        }

        Builder& labelWidth(uint16_t width) {
            labelWidth_ = width;
            return *this;
        }

        Builder& verticalLabel(bool vertical = true) {
            verticalLabel_ = vertical;
            return *this;
        }

        Builder& textAlignment(uint8_t alignment) {
            textAlignment_ = alignment;
            return *this;
        }

        Builder& valueFormat(ValueFormat format) {
            valueFormat_ = format;
            return *this;
        }

        Builder& borderMargin(uint16_t margin) {
            borderMargin_ = margin;
            return *this;
        }

        std::unique_ptr<MetricWidget> build() {
            auto widget = std::make_unique<MetricWidget>(dims_, updateIntervalMs_);

            widget->setValue(value_);
            widget->setUnit(unit_);
            widget->setRange(minValue_, maxValue_);
            widget->setColorThresholds(lowerThreshold_, upperThreshold_);
            widget->setReverseThresholds(reverseThresholds_);
            widget->setUseDimColors(useDimColors_);
            widget->setUseSmallFont(useSmallFont_);
            widget->setUseGpuColors(useGpuColors_);
            widget->setUseRamColors(useRamColors_);
            widget->setLabelColor(labelColor_);
            widget->setLabel(label_);
            widget->setLabelWidth(labelWidth_);
            widget->setVerticalLabel(verticalLabel_);
            widget->setTextAlignment(textAlignment_);
            widget->setValueFormat(valueFormat_);

            return widget;
        }

     private:
        WidgetInterface::Dimensions dims_;
        uint32_t updateIntervalMs_;

        // Configuration parameters — defaults mirror MetricWidget's own.
        int value_ = 0;
        char unit_[8] = "%";
        int minValue_ = 0;
        int maxValue_ = 100;
        float lowerThreshold_ = 50.0f;
        float upperThreshold_ = 90.0f;
        bool reverseThresholds_ = false;
        bool useDimColors_ = false;
        bool useSmallFont_ = false;
        bool useGpuColors_ = false;
        bool useRamColors_ = false;
        uint16_t labelColor_ = TFT_WHITE;
        char label_[32] = "";
        uint16_t labelWidth_ = 0;
        bool verticalLabel_ = false;
        uint8_t textAlignment_ = MC_DATUM;
        ValueFormat valueFormat_ = ValueFormat::kDefault;
        uint16_t borderMargin_ = BORDER_MARGIN;
    };

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

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
    bool useSmallFont_ = false;  // Use NotoSansDisplay15 instead of NotoSans18
    bool useGpuColors_ = false;
    bool useRamColors_ = false;
    uint16_t labelColor_ = TFT_WHITE;
    char label_[32] = "";  // Stack-allocated buffer
    uint16_t labelWidth_ = 0;
    uint8_t textAlignment_ = MC_DATUM;
    ValueFormat valueFormat_ = ValueFormat::kDefault;

    uint16_t borderMargin_ = BORDER_MARGIN;

    // State
    bool hasLabel_ = false;
    bool verticalLabel_ = false;
    int16_t valueX_ = 0;
    uint16_t valueWidth_ = 0;
    bool dimensionsDirty_ = true;
    int lastDrawnValue_ = -1;
    // Tracks "has this widget completed at least one draw since the last
    // drawStatic()?" separately from lastDrawnValue_, since -1 is also a
    // legitimate value domain for a signed int (e.g. any future metric that
    // uses -1 as a "not available" sentinel, mirroring PcMetrics::gpu_fps) —
    // using lastDrawnValue_ == -1 as the "first render" check would collide
    // with that and force a full redraw every frame instead of just once.
    bool hasDrawnOnce_ = false;
    uint16_t lastBgColor_ = TFT_BLACK;
    int16_t lastTextWidth_ = 0;
    bool valueAreaDirty_ = true;

    // Cached values for performance
    mutable char formattedValue_[24];  // Buffer for formatted value
    mutable bool formatCacheDirty_ = true;

    // Unit suffix (label font) — rendered as a second piece next to the
    // value instead of being baked into the same string/font. Width is
    // measured once (not on every draw) so the batched hot path
    // (drawValueWithLoadedFont/drawUnitWithLoadedFont) never has to load a
    // font itself. See refreshUnitWidthIfNeeded().
    mutable uint16_t unitWidthCache_ = 0;
    mutable bool unitWidthDirty_ = true;
    int16_t unitDrawX_ = 0;
    int16_t unitDrawY_ = 0;
    uint16_t unitBgColor_ = TFT_BLACK;
    bool unitNeedsRedraw_ = false;

    // Constants
    static constexpr uint16_t TEXT_MARGIN = 10;
    static constexpr uint16_t SEPARATOR_WIDTH = 1;
    static constexpr uint16_t BORDER_MARGIN = 1;

    // Load/unload the correct value font based on useSmallFont_.
    // All three render paths (renderValueArea, renderValueTextOnly,
    // drawValueWithLoadedFont) must use the same font to stay consistent.
    void loadValueFont()   const;
    void unloadValueFont() const;

    // Rendering methods
    void renderValueArea();
    void renderValueTextOnly();
    void drawDebugPixel(int16_t startX, int16_t textY, int16_t width);

    // Helper methods
    uint16_t calculateBackgroundColor() const;
    void updateDimensionCache();
    const char* getFormattedValueText() const;
    const char* getUnitText() const;
    void refreshUnitWidthIfNeeded() const;
    void safeStringCopy(char* dest, const char* src, size_t destSize) const;

    // Start X of the [value][unit] block for the current textAlignment_,
    // given the value area bounds and the combined value+unit width.
    int16_t computeStartX(int16_t areaX, int16_t areaWidth, int16_t totalW) const;

    // True when the value text's width changed enough to require clearing
    // the old glyphs before redrawing — see the callers for why a unit
    // suffix changes the shrink-only check to any-change.
    bool layoutShifted(int16_t newTextWidth, int16_t unitWidth) const;
};