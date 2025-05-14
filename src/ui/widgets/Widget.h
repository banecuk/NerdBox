#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <Arduino.h>

#include "ui/widgets/IWidget.h"

class Widget : public IWidget {
   public:
    Widget(const Dimensions& dims, uint32_t updateIntervalMs);

    void initialize(LGFX* lcd, ILogger& logger) override;
    void drawStatic() override;
    void cleanUp() override;
    void setUpdateInterval(uint32_t intervalMs) override;
    bool needsUpdate() const override;
    Dimensions getDimensions() const override;

   protected:
    LGFX* lcd_ = nullptr;
    ILogger* logger_ = nullptr;
    Dimensions dimensions_;
    uint32_t updateIntervalMs_;
    uint32_t lastUpdateTimeMs_ = 0;
    bool initialized_ = false;
    bool staticDrawn_ = false;
};

#endif  // BASEWIDGET_H