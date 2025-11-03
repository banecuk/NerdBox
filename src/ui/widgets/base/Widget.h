#pragma once

#include <Arduino.h>

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

    // Data freshness
    void markDataStale() override {
        isStale_ = true;
        isDirty_ = true;
    }
    bool isDataStale() const override { return isStale_; }
    void markDataFresh() override { isStale_ = false; }

    // Touch handling
    bool handleTouch(uint16_t x, uint16_t y) override { return false; }

    // Dimensions
    Dimensions getDimensions() const override { return dimensions_; }

 protected:
    // Protected accessors for derived classes
    LGFX* getLcd() const { return lcd_; }
    LoggerInterface* getLogger() const { return logger_; }
    DisplayContext& getContext() const { return *context_; }

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
    bool isStale_ = false;
    bool isStaticDrawn_ = false;  // Track if static elements are drawn
    bool isInitialized_ = false;  // Track initialization status

 private:
    bool canDraw() const;

    // Prevent copying
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
};