#include "WidgetManager.h"

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

    widgetCache_.push_back(std::move(entry));
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
        entry.widget->draw(true);
    }
    lcd_->endWrite();

    isInitialized_ = true;
}

void WidgetManager::updateAndDrawWidgets(bool forceRedraw) {
    if (!lcd_) {
        if (forceRedraw) {
            logger_.error("Cannot update/draw widgets: LGFX pointer is null.");
        }
        return;
    }

    if (!isInitialized_) {
        logger_.error("Cannot update/draw widgets: Not initialized.");
        return;
    }

    lcd_->startWrite();
    for (auto& entry : widgetCache_) {
        if (!entry.widget->isValid()) {
            continue;
        }

        bool needsDraw = forceRedraw || entry.widget->isDirty() || entry.widget->needsUpdate();
        if (needsDraw && entry.widget->isVisible()) {
            if (forceRedraw || entry.widget->isDirty()) {
                entry.widget->drawStatic();
            }
            entry.widget->draw(forceRedraw || entry.widget->isDirty());
        }
    }
    lcd_->endWrite();
}

void WidgetManager::updateDirtyWidgets() {
    if (!lcd_ || !isInitialized_) {
        return;
    }

    lcd_->startWrite();
    for (auto& entry : widgetCache_) {
        if (!entry.widget->isValid() || !entry.widget->isVisible()) {
            continue;
        }

        bool needsDraw = entry.widget->isDirty() || entry.widget->needsUpdate();
        if (needsDraw) {
            entry.widget->draw(entry.widget->isDirty());
        }
    }
    lcd_->endWrite();
}

void WidgetManager::markAllWidgetsDirty() {
    for (auto& entry : widgetCache_) {
        if (entry.widget->isValid() && entry.widget->isVisible()) {
            entry.widget->markDirty();
        }
    }
}

void WidgetManager::markAllWidgetsStale() {
    for (auto& entry : widgetCache_) {
        if (entry.widget->isValid()) {
            entry.widget->markDataStale();
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

    // Iterate in reverse using cached dimensions (no getDimensions() calls)
    for (auto it = widgetCache_.rbegin(); it != widgetCache_.rend(); ++it) {
        const auto& dims = it->cachedDims;  // Use cached dimensions

        // Check bounds using cached dimensions
        if (x >= dims.x && x < (dims.x + dims.width) && y >= dims.y && y < (dims.y + dims.height)) {
            if (it->widget->handleTouch(x, y)) {
                logger_.debug("Widget handled touch");
                return true;
            }
        }
    }

    logger_.debug("No widget handled the touch");
    return false;
}

void WidgetManager::cleanupWidgets() {
    isInitialized_ = false;

    for (auto& entry : widgetCache_) {
        if (entry.widget) {
            entry.widget->cleanUp();
            entry.widget.reset();
        }
    }

    widgetCache_.clear();
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
        return widgetCache_[index].widget->setVisible(visible);
    }
    return false;
}

void WidgetManager::updateCachedDimensions(size_t index) {
    if (index < widgetCache_.size()) {
        widgetCache_[index].cachedDims = widgetCache_[index].widget->getDimensions();
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
        logger_.debugf("Widget %d: %s", i, stateStr);
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