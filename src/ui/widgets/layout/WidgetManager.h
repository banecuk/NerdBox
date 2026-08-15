#pragma once

#include <memory>
#include <vector>

#include "config/LgfxConfig.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/WidgetInterface.h"
#include "utils/LoggerInterface.h"

class WidgetManager {
 public:
    explicit WidgetManager(DisplayContext& context);
    ~WidgetManager();

    void addWidget(std::unique_ptr<WidgetInterface> widget);
    void initializeWidgets();
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgetCache_.size(); }

    // Dirty management
    void markAllDirty();
    void updateDirtyWidgets();

    // Cheap pre-lock check: returns true if updateDirtyWidgets() would find
    // any work to do.  Call this before acquiring the display lock so the
    // semaphore take/give is skipped entirely on idle frames. As a side
    // effect it records each entry's chrome/value-dirty verdict, which
    // updateDirtyWidgets() consumes instead of recomputing — the two are
    // only ever called back-to-back from BaseWidgetScreen::draw(), so the
    // cached verdicts are still fresh when updateDirtyWidgets() runs.
    bool hasAnyDirtyWidgets();

 private:
    // Cache structure to avoid repeated getDimensions() calls
    struct WidgetCacheEntry {
        std::unique_ptr<WidgetInterface> widget;
        WidgetInterface::Dimensions cachedDims;
        bool isDirty = true;  // Track widget-level dirtiness
        uint32_t lastUpdateTime = 0;

        // Verdict recorded by hasAnyDirtyWidgets(), consumed by
        // updateDirtyWidgets() so it doesn't re-derive the same answers.
        bool cachedSkip = true;
        bool cachedChromeDirty = false;
        bool cachedValueDirty = false;
    };

    DisplayContext& context_;
    LoggerInterface& logger_;
    LGFX* lcd_;
    std::vector<WidgetCacheEntry> widgetCache_;
    bool isInitialized_ = false;

    bool allDirty_ = false;
    uint32_t lastStatsLogTime_ = 0;
};