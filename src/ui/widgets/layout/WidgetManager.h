#pragma once

#include <memory>
#include <set>
#include <vector>

#include "config/LgfxConfig.h"
#include "ui/core/DisplayContext.h"
#include "ui/widgets/base/WidgetInterface.h"
#include "utils/LoggerInterface.h"

class WidgetManager {
 public:
    struct DirtyRegion {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        bool isValid() const { return width > 0 && height > 0; }

        // Merge two regions
        DirtyRegion merge(const DirtyRegion& other) const {
            if (!isValid())
                return other;
            if (!other.isValid())
                return *this;

            uint16_t minX = std::min(x, other.x);
            uint16_t minY = std::min(y, other.y);
            uint16_t maxX = std::max(x + width, other.x + other.width);
            uint16_t maxY = std::max(y + height, other.y + other.height);

            return {minX, minY, static_cast<uint16_t>(maxX - minX),
                    static_cast<uint16_t>(maxY - minY)};
        }

        bool contains(const DirtyRegion& other) const {
            return x <= other.x && y <= other.y && (x + width) >= (other.x + other.width) &&
                   (y + height) >= (other.y + other.height);
        }

        bool intersects(const DirtyRegion& other) const {
            return !(x >= other.x + other.width || x + width <= other.x ||
                     y >= other.y + other.height || y + height <= other.y);
        }
    };

    explicit WidgetManager(DisplayContext& context);
    ~WidgetManager();

    void addWidget(std::unique_ptr<WidgetInterface> widget);
    void initializeWidgets();
    void updateAndDrawWidgets(bool forceRedraw = false);
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgetCache_.size(); }

    // State management methods
    std::vector<WidgetInterface::State> getWidgetStates() const;
    size_t getVisibleWidgetCount() const;
    bool setWidgetVisibility(size_t index, bool visible);

    // Dirty region management
    void markDirtyRegion(const DirtyRegion& region);
    void markAllDirty();
    void updateDirtyWidgets();  // New: Only update dirty widgets

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

    // Dirty region tracking
    std::vector<DirtyRegion> dirtyRegions_;
    bool allDirty_ = false;
    UpdateStats lastUpdateStats_;
    uint32_t lastStatsLogTime_ = 0;  // replaces the static local in updateWidgetStats

    // Helper methods
    void clearDirtyRegions();
    void addDirtyRegion(const DirtyRegion& region);
    void mergeDirtyRegions();
    bool isWidgetInDirtyRegion(const WidgetCacheEntry& entry) const;
    void updateWidgetStats(size_t dirtyCount, size_t updatedCount, size_t skippedCount);
};