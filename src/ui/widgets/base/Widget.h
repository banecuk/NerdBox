#pragma once

#include <Arduino.h>

#include "core/resources/FontRegistry.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/WidgetInterface.h"

class Widget : public WidgetInterface {
 public:
    Widget(const Dimensions& dims, uint32_t updateIntervalMs);
    virtual ~Widget();

    // Lifecycle
    void initialize(DisplayContext& context) override;
    void drawStatic() override;
    void draw(bool forceRedraw = false) override;
    void cleanUp() override;

    // State queries
    WidgetInterface::State getState() const override { return state_; }
    bool isInitialized() const override;
    bool isVisible() const override { return isVisible_; }
    bool isValid() const override;

    // Visibility control
    bool setVisible(bool visible) override;

    // Update control
    void setUpdateInterval(uint32_t intervalMs) override { updateIntervalMs_ = intervalMs; }
    bool needsUpdate() const override;

    // Dirty flags
    void markDirty() override { isDirty_ = true; }
    bool isDirty() const override { return isDirty_; }
    void clearDirty() override { isDirty_ = false; }

    // Touch handling
    bool handleTouch(uint16_t x, uint16_t y) override { return false; }

    // Dimensions
    Dimensions getDimensions() const override { return dimensions_; }

 protected:
    // Protected accessors for derived classes
    LGFX* getLcd() const { return lcd_; }
    LoggerInterface* getLogger() const { return logger_; }
    DisplayContext& getContext() const { return *context_; }

    // Small grey caption drawn top-left of the widget (e.g. "UPTIME",
    // "BRIGHTNESS"), used by every widget with a label-row-above-value/track
    // layout. Caller must already have painted the background.
    void drawCaptionLabel(const char* label, uint16_t bgColor = TFT_BLACK) {
        LGFX* lcd = getLcd();
        Fonts::loadLabel(lcd);
        lcd->setTextColor(TFT_DARKGREY, bgColor);
        lcd->setTextDatum(TL_DATUM);
        lcd->drawString(label, dimensions_.x, dimensions_.y + 2);
        Fonts::unload(lcd);
    }

    // Rounded-rect "pill" with a centered label, used by tappable
    // switch/segment widgets (SwitchWidget, BrightnessWidget) for their
    // on/off or active/inactive track.
    void drawPillToggle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t radius,
                        uint16_t bgColor, uint16_t textColor, const char* label) {
        LGFX* lcd = getLcd();
        lcd->fillRoundRect(x, y, w, h, radius, bgColor);

        Fonts::loadLabel(lcd);
        lcd->setTextColor(textColor, bgColor);
        lcd->setTextDatum(MC_DATUM);
        lcd->drawString(label, static_cast<int32_t>(x + w / 2), static_cast<int32_t>(y + h / 2));
        Fonts::unload(lcd);
    }

    // Hooks for derived classes
    virtual void onInitialize() {}
    virtual void onDrawStatic() {}
    virtual void onDraw(bool forceRedraw) = 0;
    virtual void onCleanUp() {}

    // Protected member variables
    LGFX* lcd_ = nullptr;
    LoggerInterface* logger_ = nullptr;
    DisplayContext* context_ = nullptr;

    Dimensions dimensions_;
    uint32_t updateIntervalMs_;
    uint32_t lastUpdateTimeMs_ = 0;

    // Simple state tracking
    WidgetInterface::State state_ = WidgetInterface::State::UNINITIALIZED;
    bool isVisible_ = true;
    bool isDirty_ = false;
    bool isStaticDrawn_ = false;  // Track if static elements are drawn
    bool isInitialized_ = false;  // Track initialization status

 private:
    bool canDraw() const;

    // Prevent copying
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
};