#include "WidgetManager.h"

WidgetManager::WidgetManager(ILogger& logger, LGFX* lcd) : logger_(logger), lcd_(lcd) {
    if (!lcd_) {
        logger_.error("WidgetManager created with null LGFX pointer!");
    }
}

WidgetManager::~WidgetManager() {
    logger_.debug("~WidgetManager: Cleaning up widgets");
    cleanupWidgets(); // Should safely handle empty vectors
}

void WidgetManager::addWidget(IWidget* widget) {
    if (!widget) {
        logger_.error("Null widget rejected");
        return;
    }
    widgets_.emplace_back(widget);
    logger_.debugf("[Heap] Widget post-add: %d", ESP.getFreeHeap());
    //logger_.infof("Widget added: %p", widget);
}

void WidgetManager::initializeWidgets() {
    if (!lcd_) {
        logger_.error("Cannot initialize widgets: LGFX pointer is null.");
        return;
    }
    logger_.debugf("Initializing %d widgets...", widgets_.size());
    for (auto& widget : widgets_) {
        widget->initialize(lcd_, logger_);
        widget->draw(true);
    }
    logger_.debug("Widgets initialized");
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

    if (!lcd_ || x >= lcd_->width() || y >= lcd_->height()) {
        logger_.errorf("Invalid touch coordinates: (%d, %d)", x, y);
        return false;
    }

    logger_.debugf("Checking %d widgets for touch at (%d,%d)", widgets_.size(), x, y);

    // Iterate in reverse to handle top-most widgets first
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        auto& widget = *it;
        IWidget::Dimensions dims = widget->getDimensions();

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
    if (widgets_.empty()) {
        logger_.debug("No widgets to clean up.");
        return;
    }

    logger_.debugf("Cleaning up %d widgets...", widgets_.size());
    for (auto& widget : widgets_) {
        if (widget) {
            widget->cleanUp();  // Call the widget's cleanup method
        } else {
            logger_.warning("Encountered a null widget during cleanup.");
        }
    }
    widgets_.clear();
    logger_.debug("Widget list cleared.");
}