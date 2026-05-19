#pragma once

#include "ui/core/DisplayContext.h"

class WidgetInterface {
 public:
    struct Dimensions {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;

        bool contains(uint16_t pointX, uint16_t pointY) const {
            return (pointX >= x && pointX < (x + width) && pointY >= y && pointY < (y + height));
        }

        bool isValid() const { return width > 0 && height > 0; }
    };

    // Simplified state machine - 4 essential states
    enum class State {
        UNINITIALIZED,  // Widget not yet initialized
        READY,          // Initialized and operational
        HIDDEN,         // Initialized but not visible
        ERROR           // Fatal error state
    };

    virtual ~WidgetInterface() = default;

    // Lifecycle
    virtual void initialize(DisplayContext& context) = 0;
    virtual void drawStatic() = 0;
    virtual void draw(bool forceRedraw = false) = 0;
    virtual void cleanUp() = 0;

    // State queries
    virtual State getState() const = 0;
    virtual bool isInitialized() const = 0;
    virtual bool isVisible() const = 0;
    virtual bool isValid() const = 0;

    // Visibility control
    virtual bool setVisible(bool visible) = 0;

    // Update control
    virtual void setUpdateInterval(uint32_t intervalMs) = 0;
    virtual bool needsUpdate() const = 0;

    // Dirty flags (separate from state)
    virtual void markDirty() = 0;
    virtual bool isDirty() const = 0;
    virtual void clearDirty() = 0;

    // Data freshness (separate from state)
    virtual void markDataStale() = 0;
    virtual bool isDataStale() const = 0;
    virtual void markDataFresh() = 0;

    // Touch handling
    virtual bool handleTouch(uint16_t x, uint16_t y) = 0;

    // Dimensions
    virtual Dimensions getDimensions() const = 0;
};