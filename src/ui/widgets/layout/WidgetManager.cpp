#include "WidgetManager.h"

#include <algorithm>

#include "utils/LogMacros.h"

WidgetManager::WidgetManager(DisplayContext& context)
    : context_(context), logger_(context.getLogger()), lcd_(&context.getDisplay()) {
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

    WidgetCacheEntry entry;
    entry.widget = std::move(widget);
    entry.cachedDims = entry.widget->getDimensions();
    entry.isDirty = true;  // New widgets start dirty

    widgetCache_.push_back(std::move(entry));
}

void WidgetManager::initializeWidgets() {
    if (!lcd_) {
        logger_.error("Cannot initialize widgets: LGFX pointer is null.");
        return;
    }

    lcd_->startWrite();
    for (auto& entry : widgetCache_) {
        entry.widget->initialize(context_);  // draws static chrome once internally
        entry.widget->draw(true);            // force the initial value draw
        entry.isDirty = false;               // Mark as clean after initialization
    }
    lcd_->endWrite();

    isInitialized_ = true;
}

bool WidgetManager::hasAnyDirtyWidgets() const {
    if (!isInitialized_) {
        return false;
    }
    if (allDirty_) {
        return true;
    }
    for (const auto& entry : widgetCache_) {
        if (!entry.widget->isValid() || !entry.widget->isVisible()) {
            continue;
        }
        if (entry.isDirty || entry.widget->isDirty() || entry.widget->needsUpdate()) {
            return true;
        }
    }
    return false;
}

// Only update widgets that are actually dirty — no region tracking needed.
// Widget-level dirty flags (isDirty / isDirty() / needsUpdate()) are more
// accurate and cheaper than maintaining a separate region list.
//
// Two-tier dirty model:
//   entry.isDirty  — "chrome dirty": widget was just added, screen just entered,
//                    or layout changed. Triggers drawStatic() to repaint borders,
//                    labels, and background.  Cleared here after drawStatic().
//   widget->isDirty() — "value dirty": only the dynamic content changed (a new
//                    sensor reading, clock tick, etc.).  Never triggers drawStatic().
//
// This separation is what prevents the flash seen when a periodic value update
// causes a full background clear followed by a text repaint.
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

        const bool chromeDirty = allDirty_ || entry.isDirty;
        const bool valueDirty = entry.widget->isDirty() || entry.widget->needsUpdate();

        if (!chromeDirty && !valueDirty) {
            skippedCount++;
            continue;
        }

        dirtyCount++;

        // Repaint static chrome (background, borders, labels) only when the
        // manager-level flag says so.  A plain value update must not trigger
        // this — that is what caused the flash.
        if (chromeDirty) {
            entry.widget->drawStatic();
            entry.isDirty = false;
            entry.widget->clearDirty();
        }

        entry.widget->draw(chromeDirty);  // pass forceRedraw when chrome was just cleared
        entry.lastUpdateTime = millis();
        updatedCount++;
    }

    lcd_->endWrite();

    allDirty_ = false;

    uint32_t currentTime = millis();
    if (currentTime - lastStatsLogTime_ > 10000) {  // Every 10 seconds
        float efficiency = (float)skippedCount / (float)widgetCache_.size() * 100.0f;
        LOG_DEBUGF(logger_, "WidgetManager: %zu/%zu widgets updated (%.1f%% skipped)", updatedCount,
                  widgetCache_.size(), efficiency);
        lastStatsLogTime_ = currentTime;
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

    for (auto it = widgetCache_.rbegin(); it != widgetCache_.rend(); ++it) {
        if (!it->widget->isValid() || !it->widget->isVisible()) {
            continue;
        }

        const auto& dims = it->cachedDims;
        if (dims.contains(x, y)) {
            if (it->widget->handleTouch(x, y)) {
                it->isDirty = true;
                LOG_DEBUG(logger_, "Widget handled touch and marked dirty");
                return true;
            }
        }
    }

    LOG_DEBUG(logger_, "No widget handled the touch");
    return false;
}

void WidgetManager::cleanupWidgets() {
    isInitialized_ = false;
    allDirty_ = false;

    for (auto& entry : widgetCache_) {
        if (entry.widget) {
            entry.widget->cleanUp();
            entry.widget.reset();
        }
    }

    widgetCache_.clear();
}
