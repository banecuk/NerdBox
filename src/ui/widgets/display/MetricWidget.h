#pragma once

#include <cstring>
#include <memory>

#include "ui/widgets/base/Widget.h"

class MetricWidget : public Widget {
 public:
    // Every tunable knob in one place, with default member initializers
    // matching MetricWidget's own historical defaults. Passed to the
    // constructor once instead of chained through per-field setters, so the
    // "defaults must mirror" duplication between a builder and the widget's
    // own fields can't happen.
    struct Config {
        int value = 0;
        const char* unit = "%";  // copied into a fixed buffer by the constructor
        int minValue = 0;
        int maxValue = 100;
        float lowerThreshold = 50.0f;
        float upperThreshold = 90.0f;
        bool reverseThresholds = false;
        bool useDimColors = false;
        bool useSmallFont = false;  // Use NotoSansDisplay15 instead of NotoSans18
        bool useGpuColors = false;
        bool useRamColors = false;
        uint16_t labelColor = TFT_WHITE;
        const char* label = "";  // copied into a fixed buffer by the constructor
        uint16_t labelWidth = 0;
        bool verticalLabel = false;  // stack label chars top-to-bottom — for tall tiles
        uint8_t textAlignment = MC_DATUM;  // TL_DATUM, TC_DATUM, TR_DATUM, etc.
        // Inset of the coloured value area from the widget's edges. A zero
        // margin makes the fill run flush to the widget bounds (used by
        // DiskBandWidget so its metric tiles sit immediately against the
        // activity lines).
        uint16_t borderMargin = 1;
    };

    MetricWidget(const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs,
                 const Config& config);

    bool handleTouch(uint16_t x, uint16_t y) override;

    void setValue(int value);

    // Called by a batch-drawing caller (PcMetricsWidget, DiskBandWidget):
    // assumes the value font matching useSmallFont_ (NotoSans18 via
    // loadMetric(), or NotoSansDisplay15 via loadValue() for small-font
    // tiles) is already loaded by the caller. Skips loadFont/unloadFont
    // overhead. Callers must batch tiles of the same useSmallFont_ setting
    // together so one font load covers the whole pass.
    void drawValueWithLoadedFont();

    // Paired with drawValueWithLoadedFont() — call after it, with the label
    // font (NotoSansDisplay12) loaded by the caller, to draw the unit
    // suffix. Only redraws when the preceding drawValueWithLoadedFont() call
    // actually moved or recoloured it; a no-op otherwise.
    void drawUnitWithLoadedFont();

    void forceRefresh();

    // Getters
    int getValue() const { return value_; }
    int getMinValue() const { return minValue_; }
    int getMaxValue() const { return maxValue_; }
    const char* getUnit() const { return unit_; }
    float getLowerThreshold() const { return lowerThreshold_; }
    float getUpperThreshold() const { return upperThreshold_; }
    bool getUseDimColors() const { return useDimColors_; }
    bool isSmallFont() const { return useSmallFont_; }

 protected:
    void onDraw(bool forceRedraw) override;
    void onDrawStatic() override;

 private:
    // Configuration
    int value_;
    int minValue_;
    int maxValue_;
    char unit_[8];  // Stack-allocated buffer
    float lowerThreshold_;
    float upperThreshold_;
    bool reverseThresholds_;
    bool useDimColors_;
    bool useSmallFont_;
    bool useGpuColors_;
    bool useRamColors_;
    uint16_t labelColor_;
    char label_[32];  // Stack-allocated buffer
    uint16_t labelWidth_;
    uint8_t textAlignment_;
    uint16_t borderMargin_;
    bool hasLabel_;
    bool verticalLabel_;

    // State
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

    // Load/unload the correct value font based on useSmallFont_.
    // All three render paths (renderValueArea, renderValueTextOnly,
    // drawValueWithLoadedFont) must use the same font to stay consistent.
    void loadValueFont() const;
    void unloadValueFont() const;

    // Rendering methods
    void renderValueArea();
    void renderValueTextOnly();

    // Helper methods
    uint16_t calculateBackgroundColor() const;
    void updateDimensionCache();
    const char* getFormattedValueText() const;
    void refreshUnitWidthIfNeeded() const;
    void safeStringCopy(char* dest, const char* src, size_t destSize) const;

    // Value area bounds for the current hasLabel_/borderMargin_ — was
    // computed via a hasLabel_ if/else copy-pasted at every render-path call
    // site; now the single place any of them read it from.
    void getValueAreaBounds(int16_t& areaX, int16_t& areaY, int16_t& areaWidth,
                             int16_t& areaHeight) const;

    // setTextColor/setTextDatum/drawString triple shared by every value- and
    // unit-suffix draw across all three render paths. Caller owns loading
    // the correct font first.
    void drawValueText(const char* text, int16_t x, int16_t y, uint16_t bgColor);

    // Start X of the [value][unit] block for the current textAlignment_,
    // given the value area bounds and the combined value+unit width.
    int16_t computeStartX(int16_t areaX, int16_t areaWidth, int16_t totalW) const;

    // True when the value text's width changed enough to require clearing
    // the old glyphs before redrawing — see the callers for why a unit
    // suffix changes the shrink-only check to any-change.
    bool layoutShifted(int16_t newTextWidth, int16_t unitWidth) const;
};
