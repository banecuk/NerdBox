#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <memory>
#include <vector>

#include "config/LgfxConfig.h"
#include "ui/widgets/IWidget.h"
#include "utils/ILogger.h"

class WidgetManager {
   public:
    explicit WidgetManager(ILogger& logger, LGFX* lcd);
    ~WidgetManager();

    // Takes ownership of the widget via unique_ptr
    void addWidget(std::unique_ptr<IWidget> widget);
    void initializeWidgets();
    void updateAndDrawWidgets(bool forceRedraw = false);
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgets_.size(); }

   private:
    ILogger& logger_;
    LGFX* lcd_;
    std::vector<std::unique_ptr<IWidget>> widgets_;
};

#endif  // WIDGETMANAGER_H