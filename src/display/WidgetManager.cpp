#include "WidgetManager.h"

WidgetManager::WidgetManager(ILogger& logger, LGFX* lcd) 
    : logger_(logger), lcd_(lcd) {
    if (!lcd_) {
        logger_.error("WidgetManager created with null LGFX pointer!");
    }
}

WidgetManager::~WidgetManager() {
    cleanupWidgets();
}

void WidgetManager::addWidget(std::unique_ptr<IWidget> widget) {
    if (widget) {
        widgets_.push_back(std::move(widget));
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
    for (auto& widget : widgets_) {
        widget->initialize(lcd_, logger_);
    }
}

void WidgetManager::updateAndDrawWidgets(bool forceRedraw) {
    if (!lcd_) {
        if (forceRedraw) {
            logger_.error("Cannot update/draw widgets: LGFX pointer is null.");
        }
        return;
    }

    for (auto& widget : widgets_) {
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

    // Iterate in reverse to handle top-most widgets first
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        auto& widget = *it;
        IWidget::Dimensions dims = widget->getDimensions();

        if (x >= dims.x && x < (dims.x + dims.width) && 
            y >= dims.y && y < (dims.y + dims.height)) {
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
    for (auto& widget : widgets_) {
        widget->cleanUp();
    }
    widgets_.clear();
    logger_.debug("Widget list cleared.");
}