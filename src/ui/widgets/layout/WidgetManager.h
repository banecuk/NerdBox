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
    void updateAndDrawWidgets(bool forceRedraw = false);
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgetCache_.size(); }

    // State management methods
    std::vector<WidgetInterface::State> getWidgetStates() const;
    size_t getVisibleWidgetCount() const;
    bool setWidgetVisibility(size_t index, bool visible);

    void updateDirtyWidgets();
    void markAllWidgetsDirty();
    void markAllWidgetsStale();

    // Update cached dimensions if widget resizes
    void updateCachedDimensions(size_t index);

    // Debug methods
    void logWidgetStates() const;
    size_t getWidgetsInState(WidgetInterface::State state) const;

 private:
    // Cache structure to avoid repeated getDimensions() calls
    struct WidgetCacheEntry {
        std::unique_ptr<WidgetInterface> widget;
        WidgetInterface::Dimensions cachedDims;
    };

    DisplayContext& context_;
    LoggerInterface& logger_;
    LGFX* lcd_;
    std::vector<WidgetCacheEntry> widgetCache_;
    bool isInitialized_ = false;
};