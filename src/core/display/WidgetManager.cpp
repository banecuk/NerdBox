#include "WidgetManager.h"

WidgetManager::WidgetManager(ILogger& logger, LGFX* lcd) : logger_(logger), lcd_(lcd) {
    if (!lcd_) {
        logger_.error("WidgetManager created with null LGFX pointer!");
    }
}

WidgetManager::~WidgetManager() {
    if (!widgets_.empty()) {
        logger_.warning(
            "WidgetManager destroyed while still holding widgets. Call "
            "cleanupWidgets() "
            "explicitly.");
        widgets_.clear();
    }
}

void WidgetManager::addWidget(IWidget* widget) {
    if (widget) {
        widgets_.push_back(widget);
    } else {
        logger_.warning("Attempted to add null widget.");
    }
}

void WidgetManager::initializeWidgets() {
    if (!lcd_) {
        logger_.error("Cannot initialize widgets: LGFX pointer is null.");
        return;
    }
    logger_.debug("Initializing %d widgets...", widgets_.size());
    for (IWidget* widget : widgets_) {
        widget->initialize(lcd_, logger_);
    }
}

void WidgetManager::updateAndDrawWidgets(bool forceRedraw /* = false */) {
    if (!lcd_) {
        if (forceRedraw) {
            logger_.error("Cannot update/draw widgets: LGFX pointer is null.");
        }
        return;
    }

    for (IWidget* widget : widgets_) {
        bool needsDraw = forceRedraw || widget->needsUpdate();

        if (needsDraw) {
            widget->draw(forceRedraw);
        }
    }
}

bool WidgetManager::handleTouch(uint16_t x, uint16_t y) {
    if (!lcd_) {
        logger_.error("Cannot handle touch - LCD not set");
        return false;
    }

    logger_.debugf("Checking %d widgets for touch at (%d,%d)", widgets_.size(), x, y);

    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        IWidget* widget = *it;
        IWidget::Dimensions dims = widget->getDimensions();

        logger_.debugf("Checking widget at (%d,%d %dx%d)", dims.x, dims.y, dims.width,
                       dims.height);

        if (x >= dims.x && x < (dims.x + dims.width) && y >= dims.y &&
            y < (dims.y + dims.height)) {
            logger_.debugf("Widget found at (%d,%d)", x, y);
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
    logger_.debugf("Cleaning up %d widgets...", widgets_.size());
    for (IWidget* widget : widgets_) {
        widget->cleanUp();
    }
    // Clear the list of pointers - does NOT delete the actual widget objects
    widgets_.clear();
    logger_.debug("Widget list cleared.");
}