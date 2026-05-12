#include "WidgetManager.h"

#include <algorithm>

WidgetManager::WidgetManager(DisplayContext& context)
    : logger_(context.getLogger()), lcd_(&context.getDisplay()), context_(context) {
    if (!lcd_) {
        logger_.error("WidgetManager created with null LGFX pointer!");
    }
}

WidgetManager::~WidgetManager() {
    cleanupWidgets();
}

void WidgetManager::addWidget(std::unique_ptr<WidgetInterface> widget) {
    if (!widget) {
        logger_.error("Null widget rejected");
        return;
    }

    // Cache dimensions on add to avoid repeated getDimensions() calls
    WidgetCacheEntry entry;
    entry.widget = std::move(widget);
    entry.cachedDims = entry.widget->getDimensions();
    entry.isDirty = true;  // New widgets start dirty

    widgetCache_.push_back(std::move(entry));

    // Mark the new widget's region as dirty
    markDirtyRegion(
        {entry.cachedDims.x, entry.cachedDims.y, entry.cachedDims.width, entry.cachedDims.height});
}

void WidgetManager::initializeWidgets() {
    if (!lcd_) {
        logger_.error("Cannot initialize widgets: LGFX pointer is null.");
        return;
    }

    lcd_->startWrite();
    for (auto& entry : widgetCache_) {
        entry.widget->initialize(context_);
        entry.widget->drawStatic();
        entry.widget->draw(true);  // Force initial draw
        entry.isDirty = false;     // Mark as clean after initialization
    }
    lcd_->endWrite();

    isInitialized_ = true;
    clearDirtyRegions();  // Start with clean slate
}

// OPTIMIZED: Only update dirty widgets
void WidgetManager::updateDirtyWidgets() {
    if (!lcd_ || !isInitialized_) {
        return;
    }

    size_t dirtyCount = 0;
    size_t updatedCount = 0;
    size_t skippedCount = 0;

    lcd_->startWrite();

    for (auto& entry : widgetCache_) {
        if (!entry.widget->isValid() || !entry.widget->isVisible()) {
            skippedCount++;
            continue;
        }

        // Check if widget needs update (dirty flag, needsUpdate, or in dirty region)
        bool isWidgetDirty = entry.isDirty || entry.widget->isDirty() ||
                             entry.widget->needsUpdate() ||
                             (allDirty_ || isWidgetInDirtyRegion(entry));

        if (isWidgetDirty) {
            dirtyCount++;

            if (entry.widget->isDirty() || entry.isDirty) {
                entry.widget->drawStatic();
                entry.isDirty = false;
                entry.widget->clearDirty();
            }

            entry.widget->draw(false);
            entry.lastUpdateTime = millis();
            updatedCount++;
        } else {
            skippedCount++;
        }
    }

    lcd_->endWrite();

    // Clear dirty regions after processing
    if (!allDirty_) {
        clearDirtyRegions();
    }

    updateWidgetStats(dirtyCount, updatedCount, skippedCount);
}

// Legacy method - now uses optimized version
void WidgetManager::updateAndDrawWidgets(bool forceRedraw) {
    if (forceRedraw) {
        markAllDirty();
    }
    updateDirtyWidgets();
}

void WidgetManager::markDirtyRegion(const DirtyRegion& region) {
    if (region.isValid()) {
        dirtyRegions_.push_back(region);

        // Limit the number of dirty regions to prevent memory issues
        if (dirtyRegions_.size() > 20) {
            mergeDirtyRegions();
        }
    }
}

void WidgetManager::markAllDirty() {
    allDirty_ = true;
    for (auto& entry : widgetCache_) {
        entry.isDirty = true;
        if (entry.widget->isValid()) {
            entry.widget->markDirty();
        }
    }
}

void WidgetManager::clearDirtyRegions() {
    dirtyRegions_.clear();
    allDirty_ = false;
}

void WidgetManager::addDirtyRegion(const DirtyRegion& region) {
    if (!region.isValid())
        return;

    dirtyRegions_.push_back(region);
}

void WidgetManager::mergeDirtyRegions() {
    if (dirtyRegions_.size() <= 1)
        return;

    std::vector<DirtyRegion> merged;
    merged.push_back(dirtyRegions_[0]);

    for (size_t i = 1; i < dirtyRegions_.size(); ++i) {
        bool mergedWithExisting = false;

        for (auto& existing : merged) {
            if (existing.intersects(dirtyRegions_[i])) {
                existing = existing.merge(dirtyRegions_[i]);
                mergedWithExisting = true;
                break;
            }
        }

        if (!mergedWithExisting) {
            merged.push_back(dirtyRegions_[i]);
        }
    }

    // Limit to reasonable number
    if (merged.size() > 10) {
        DirtyRegion superRegion = merged[0];
        for (size_t i = 1; i < merged.size(); ++i) {
            superRegion = superRegion.merge(merged[i]);
        }
        dirtyRegions_ = {superRegion};
    } else {
        dirtyRegions_ = merged;
    }
}

bool WidgetManager::isWidgetInDirtyRegion(const WidgetCacheEntry& entry) const {
    if (allDirty_ || dirtyRegions_.empty()) {
        return allDirty_;
    }

    DirtyRegion widgetRegion = {entry.cachedDims.x, entry.cachedDims.y, entry.cachedDims.width,
                                entry.cachedDims.height};

    for (const auto& dirtyRegion : dirtyRegions_) {
        if (dirtyRegion.intersects(widgetRegion)) {
            return true;
        }
    }

    return false;
}

void WidgetManager::updateWidgetStats(size_t dirtyCount, size_t updatedCount, size_t skippedCount) {
    lastUpdateStats_.totalWidgets = widgetCache_.size();
    lastUpdateStats_.dirtyWidgets = dirtyCount;
    lastUpdateStats_.updatedWidgets = updatedCount;
    lastUpdateStats_.skippedWidgets = skippedCount;

    // Log performance improvements occasionally
    uint32_t currentTime = millis();
    if (currentTime - lastStatsLogTime_ > 10000) {  // Every 10 seconds
        float efficiency = (float)skippedCount / (float)widgetCache_.size() * 100.0f;
        logger_.debugf("WidgetManager: %zu/%zu widgets updated (%.1f%% skipped)", updatedCount,
                       widgetCache_.size(), efficiency);
        lastStatsLogTime_ = currentTime;
    }
}

// Rest of the existing methods remain mostly the same, but optimized...

bool WidgetManager::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_ || !lcd_->width() || !lcd_->height()) {
        logger_.error("LCD not properly initialized");
        return false;
    }

    if (x >= lcd_->width() || y >= lcd_->height()) {
        logger_.errorf("Invalid touch coordinates: (%d, %d)", x, y);
        return false;
    }

    if (!isInitialized_) {
        logger_.error("Cannot handle touch: Not initialized.");
        return false;
    }

    // OPTIMIZED: Only check visible and valid widgets
    // Iterate in reverse using cached dimensions
    for (auto it = widgetCache_.rbegin(); it != widgetCache_.rend(); ++it) {
        if (!it->widget->isValid() || !it->widget->isVisible()) {
            continue;
        }

        const auto& dims = it->cachedDims;
        if (dims.contains(x, y)) {
            if (it->widget->handleTouch(x, y)) {
                // Mark widget as dirty if it handled the touch
                it->isDirty = true;
                markDirtyRegion({dims.x, dims.y, dims.width, dims.height});
                logger_.debug("Widget handled touch and marked dirty");
                return true;
            }
        }
    }

    logger_.debug("No widget handled the touch");
    return false;
}

void WidgetManager::cleanupWidgets() {
    isInitialized_ = false;
    clearDirtyRegions();

    for (auto& entry : widgetCache_) {
        if (entry.widget) {
            entry.widget->cleanUp();
            entry.widget.reset();
        }
    }

    widgetCache_.clear();
}

// Update other methods to maintain widget dirty state...

void WidgetManager::markAllWidgetsDirty() {
    markAllDirty();  // Now uses the optimized version
}

std::vector<WidgetInterface::State> WidgetManager::getWidgetStates() const {
    std::vector<WidgetInterface::State> states;
    for (const auto& entry : widgetCache_) {
        states.push_back(entry.widget->getState());
    }
    return states;
}

size_t WidgetManager::getVisibleWidgetCount() const {
    size_t count = 0;
    for (const auto& entry : widgetCache_) {
        if (entry.widget->isVisible()) {
            count++;
        }
    }
    return count;
}

bool WidgetManager::setWidgetVisibility(size_t index, bool visible) {
    if (index < widgetCache_.size()) {
        bool changed = widgetCache_[index].widget->setVisible(visible);
        if (changed) {
            // Mark widget as dirty when visibility changes
            widgetCache_[index].isDirty = true;
            markDirtyRegion({widgetCache_[index].cachedDims.x, widgetCache_[index].cachedDims.y,
                             widgetCache_[index].cachedDims.width,
                             widgetCache_[index].cachedDims.height});
        }
        return changed;
    }
    return false;
}

void WidgetManager::updateCachedDimensions(size_t index) {
    if (index < widgetCache_.size()) {
        widgetCache_[index].cachedDims = widgetCache_[index].widget->getDimensions();
        // Mark as dirty since dimensions changed
        widgetCache_[index].isDirty = true;
        markDirtyRegion({widgetCache_[index].cachedDims.x, widgetCache_[index].cachedDims.y,
                         widgetCache_[index].cachedDims.width,
                         widgetCache_[index].cachedDims.height});
    }
}

void WidgetManager::logWidgetStates() const {
    for (size_t i = 0; i < widgetCache_.size(); ++i) {
        auto state = widgetCache_[i].widget->getState();
        const char* stateStr = "UNKNOWN";
        switch (state) {
            case WidgetInterface::State::UNINITIALIZED:
                stateStr = "UNINITIALIZED";
                break;
            case WidgetInterface::State::READY:
                stateStr = "READY";
                break;
            case WidgetInterface::State::HIDDEN:
                stateStr = "HIDDEN";
                break;
            case WidgetInterface::State::ERROR:
                stateStr = "ERROR";
                break;
        }
        logger_.debugf("Widget %d: %s (dirty: %s)", i, stateStr,
                       widgetCache_[i].isDirty ? "yes" : "no");
    }
}

size_t WidgetManager::getWidgetsInState(WidgetInterface::State state) const {
    size_t count = 0;
    for (const auto& entry : widgetCache_) {
        if (entry.widget->getState() == state) {
            count++;
        }
    }
    return count;
}