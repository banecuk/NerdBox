#pragma once

#include <Arduino.h>

#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/WidgetInterface.h"

class Widget : public WidgetInterface {
 public:
    Widget(const Dimensions& dims, uint32_t updateIntervalMs);
    // Deliberately does NOT call cleanUp(): from inside a destructor the
    // virtual onCleanUp() would resolve to this base's no-op, so subclass
    // cleanup never ran this way anyway. WidgetManager::cleanupWidgets() is
    // the one path that calls cleanUp() — before destroying each widget.
    virtual ~Widget() = default;

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

    // One-shot wake-up: makes needsUpdate() report true once `ms` has
    // elapsed, independent of (and without touching) updateIntervalMs_ — for
    // a widget that is otherwise static (interval 0) but needs one more draw
    // later, e.g. ButtonWidget releasing its pressed state. Cleared when the
    // draw it triggers runs; calling it again re-arms it.
    void requestWakeAfter(uint32_t ms);

    // Hooks for derived classes
    virtual void onInitialize() {}
    virtual void onDrawStatic() {}
    // `fullRepaint` is true when the draw was forced by the caller *or* the
    // widget was marked dirty (Widget::draw() passes `forceRedraw ||
    // isDirty_`): repaint everything, don't diff against cached state.
    // Widget::draw() clears the dirty flag after this returns, so overrides
    // must not call clearDirty() themselves. Subclasses still name the
    // parameter forceRedraw — same meaning.
    virtual void onDraw(bool fullRepaint) = 0;
    virtual void onCleanUp() {}

    // Geometry and scheduling state subclasses legitimately read in
    // onDraw()/onDrawStatic(). The display/logger/context pointers are
    // private below — use getLcd()/getLogger()/getContext().
    Dimensions dimensions_;
    uint32_t updateIntervalMs_;
    bool isStaticDrawn_ = false;  // Track if static elements are drawn

 private:
    bool canDraw() const;
    bool wakeDue() const;

    // Supplied by initialize(), cleared by cleanUp(). Private so a subclass
    // can't shadow or reseat them (ThreadsWidget/DiskInfoWidget used to keep
    // their own constructor-injected DisplayContext& context_).
    LGFX* lcd_ = nullptr;
    LoggerInterface* logger_ = nullptr;
    DisplayContext* context_ = nullptr;

    // requestWakeAfter() state.
    uint32_t wakeRequestedAtMs_ = 0;
    uint32_t wakeAfterMs_ = 0;
    bool wakePending_ = false;

    // Written only by Widget itself — draw() stamps lastUpdateTimeMs_ and
    // clears isDirty_ after every onDraw(), so a subclass never needs to.
    // Use markDirty()/isDirty()/isVisible()/isInitialized() instead.
    uint32_t lastUpdateTimeMs_ = 0;
    WidgetInterface::State state_ = WidgetInterface::State::UNINITIALIZED;
    bool isVisible_ = true;
    bool isDirty_ = false;
    bool isInitialized_ = false;

    // Prevent copying
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
};