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

    // State management methods
    std::vector<WidgetInterface::State> getWidgetStates() const;
    size_t getVisibleWidgetCount() const;
    bool setWidgetVisibility(size_t index, bool visible);

    // Dirty management
    void markAllDirty();
    void updateDirtyWidgets();

    // Cheap pre-lock check: returns true if updateDirtyWidgets() would find
    // any work to do.  Call this before acquiring the display lock so the
    // semaphore take/give is skipped entirely on idle frames.
    bool hasAnyDirtyWidgets() const;

    void markAllWidgetsDirty();
    void markAllWidgetsStale();

    // Update cached dimensions if widget resizes
    void updateCachedDimensions(size_t index);

    // Debug methods
    void logWidgetStates() const;
    size_t getWidgetsInState(WidgetInterface::State state) const;

    // Performance monitoring
    struct UpdateStats {
        size_t totalWidgets = 0;
        size_t dirtyWidgets = 0;
        size_t updatedWidgets = 0;
        size_t skippedWidgets = 0;
    };

    UpdateStats getLastUpdateStats() const { return lastUpdateStats_; }

 private:
    // Cache structure to avoid repeated getDimensions() calls
    struct WidgetCacheEntry {
        std::unique_ptr<WidgetInterface> widget;
        WidgetInterface::Dimensions cachedDims;
        bool isDirty = true;  // Track widget-level dirtiness
        uint32_t lastUpdateTime = 0;
    };

    DisplayContext& context_;
    LoggerInterface& logger_;
    LGFX* lcd_;
    std::vector<WidgetCacheEntry> widgetCache_;
    bool isInitialized_ = false;

    bool allDirty_ = false;
    UpdateStats lastUpdateStats_;
    uint32_t lastStatsLogTime_ = 0;

    void updateWidgetStats(size_t dirtyCount, size_t updatedCount, size_t skippedCount);
};