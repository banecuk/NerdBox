#pragma once

#include <memory>
#include <vector>

#include "config/LgfxConfig.h"
#include "ui/DisplayContext.h"
#include "ui/widgets/WidgetInterface.h"
#include "utils/LoggerInterface.h"

class WidgetManager {
   public:
    explicit WidgetManager(DisplayContext& context);
    ~WidgetManager();

    void addWidget(std::unique_ptr<WidgetIterface> widget);
    void initializeWidgets();
    void updateAndDrawWidgets(bool forceRedraw = false);
    bool handleTouch(uint16_t x, uint16_t y);
    void cleanupWidgets();
    size_t getWidgetCount() const { return widgets_.size(); }

   private:
    DisplayContext& context_;
    LoggerInterface& logger_;
    LGFX* lcd_;
    std::vector<std::unique_ptr<WidgetIterface>> widgets_;
    bool initialized_ = false;
};