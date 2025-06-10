#pragma once

#include "ui/DisplayContext.h"
#include "ui/screens/ScreenInterface.h"

class WidgetIterface {
   public:
    struct Dimensions {
        uint16_t x;       // X position (pixels)
        uint16_t y;       // Y position (pixels)
        uint16_t width;   // Width (pixels)
        uint16_t height;  // Height (pixels)
    };

    virtual ~WidgetIterface() = default;

    // Core methods
    virtual void initialize(DisplayContext& context) = 0;
    virtual void drawStatic() = 0;
    virtual void draw(bool forceRedraw = false) = 0;
    virtual void cleanUp() = 0;

    // Update control
    virtual void setUpdateInterval(uint32_t intervalMs) = 0;
    virtual bool needsUpdate() const = 0;

    // Touch handling
    virtual bool handleTouch(uint16_t x, uint16_t y) = 0;

    // Immutable dimensions access
    virtual Dimensions getDimensions() const = 0;
};
