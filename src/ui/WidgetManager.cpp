#include "WidgetManager.h"

#include <esp_task_wdt.h>

WidgetManager::WidgetManager(DisplayContext& context)
    : logger_(context.getLogger()), lcd_(&context.getDisplay()), context_(context) {
    if (!lcd_) {
        logger_.error("WidgetManager created with null LGFX pointer!");
    }
}

WidgetManager::~WidgetManager() {
    // logger_.debug("~WidgetManager: Cleaning up widgets");
    cleanupWidgets();
}

void WidgetManager::addWidget(std::unique_ptr<WidgetIterface> widget) {
    if (!widget) {
        logger_.error("Null widget rejected");
        return;
    }
    widgets_.push_back(std::move(widget));
    // logger_.debugf("[Heap] Widget post-add: %d", ESP.getFreeHeap());
}

void WidgetManager::initializeWidgets() {
    if (!lcd_) {
        logger_.error("Cannot initialize widgets: LGFX pointer is null.");
        return;
    }
    // logger_.debugf("Initializing %d widgets", widgets_.size());

    lcd_->startWrite();
    for (auto& widget : widgets_) {
        widget->initialize(context_);
        widget->drawStatic();
        widget->draw(true);
    }
    lcd_->endWrite();

    // logger_.debug("Widgets initialized");
    initialized_ = true;
}

void WidgetManager::updateAndDrawWidgets(bool forceRedraw) {
    if (!lcd_) {
        if (forceRedraw) {
            logger_.error("Cannot update/draw widgets: LGFX pointer is null.");
        }
        return;
    }

    if (!initialized_) {
        logger_.error("Cannot update/draw widgets: Not initialized.");
        return;
    }

    lcd_->startWrite();
    for (auto& widget : widgets_) {
        bool needsDraw = forceRedraw || widget->needsUpdate();
        if (needsDraw) {
            // logger_.debugf("Drawing widget at (%d, %d)", widget->getDimensions().x,
            //                 widget->getDimensions().y);
            if (forceRedraw) {
                widget->drawStatic();
            }
            widget->draw(forceRedraw);
        }
    }
    lcd_->endWrite();
}

bool WidgetManager::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_ || !lcd_->width() || !lcd_->height()) {
        logger_.error("LCD not properly initialized");
        return false;
    }

    if (!lcd_ || x >= lcd_->width() || y >= lcd_->height()) {
        logger_.errorf("Invalid touch coordinates: (%d, %d)", x, y);
        return false;
    }

    if (!initialized_) {
        logger_.error("Cannot handle touch: Not initialized.");
        return false;
    }

    // logger_.debugf("Checking %d widgets for touch at (%d,%d)", widgets_.size(), x, y);

    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        auto& widget = *it;
        WidgetIterface::Dimensions dims = widget->getDimensions();

        if (x >= dims.x && x < (dims.x + dims.width) && y >= dims.y &&
            y < (dims.y + dims.height)) {
            // logger_.debugf("Widget found at (%d,%d)", x, y);
            if (widget->handleTouch(x, y)) {
                logger_.debug("Widget handled touch");
                return true;
            }
        }
    }
    logger_.debug("No widget handled the touch");
    return false;
}

void WidgetManager::cleanupWidgets() {
    // logger_.debugf("Clearing %d widgets", widgets_.size());

    initialized_ = false;  // Mark as uninitialized before cleanup

    for (auto& widget : widgets_) {
        if (widget) {
            widget->cleanUp();  // Ensure all widgets clean up their resources
            widget.reset();     // Explicitly release
        }
    }

    widgets_.clear();  // Then clear the container
    // logger_.debugf("Widgets cleared. Count: %d", widgets_.size());
    // logger_.debug("Widgets cleared waiting done.");
}