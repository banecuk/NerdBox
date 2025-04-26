#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <Arduino.h>

#include "display/widgets/IWidget.h"  // Include the interface

class BaseWidget : public IWidget {
   public:
    // Constructor takes dimensions and update interval
    BaseWidget(const Dimensions& dims, uint32_t updateIntervalMs);

    // --- IWidget Overrides ---
    void initialize(LGFX* lcd, ILogger& logger) override;
    // draw() remains pure virtual - concrete widgets MUST implement drawing
    // virtual void draw(bool forceRedraw = false) = 0;
    void cleanUp() override;  // Provide a default empty cleanup

    void setUpdateInterval(uint32_t intervalMs) override;
    bool needsUpdate() const override;

    // handleTouch() remains pure virtual - concrete widgets MUST implement touch handling
    // virtual bool handleTouch(uint16_t x, uint16_t y) = 0;

    Dimensions getDimensions() const override;

   protected:
    // Make lcd and logger accessible to derived classes
    LGFX* lcd_ = nullptr;
    ILogger* logger_ = nullptr;
    Dimensions dimensions_;
    uint32_t updateIntervalMs_;
    uint32_t lastUpdateTimeMs_ = 0;
    bool initialized_ = false;
};

#endif  // BASEWIDGET_H