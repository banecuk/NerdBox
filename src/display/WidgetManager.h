// WidgetManager.h
#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <LovyanGFX.hpp>
#include <vector>

#include "config/LgfxConfig.h"  // For LGFX type
#include "utils/ILogger.h"
#include "display/widgets/IWidget.h"

class WidgetManager {
   public:
    explicit WidgetManager(ILogger& logger, LGFX* lcd);
    ~WidgetManager();

    void addWidget(IWidget* widget);
    void initializeWidgets();

    // Updates widgets needing timed updates OR draws all if forceRedraw is true.
    void updateAndDrawWidgets(bool forceRedraw = false);

    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();

   private:
    ILogger& logger_;
    LGFX* lcd_;
    std::vector<IWidget*> widgets_;
};

#endif  // WIDGETMANAGER_H