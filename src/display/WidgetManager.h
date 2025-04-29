#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include <vector>
#include <memory>  // For std::unique_ptr
#include "config/LgfxConfig.h"
#include "utils/ILogger.h"
#include "display/widgets/IWidget.h"

class WidgetManager {
public:
    explicit WidgetManager(ILogger& logger, LGFX* lcd);
    ~WidgetManager();

    // Takes ownership of the widget
    void addWidget(std::unique_ptr<IWidget> widget);
    void initializeWidgets();
    void updateAndDrawWidgets(bool forceRedraw = false);
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();

private:
    ILogger& logger_;
    LGFX* lcd_;
    std::vector<std::unique_ptr<IWidget>> widgets_;
};

#endif  // WIDGETMANAGER_H