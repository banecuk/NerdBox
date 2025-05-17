#pragma once

#include <Arduino.h>

#include "ui/widgets/WidgetInterface.h"

class Widget : public WidgetIterface {
   public:
    Widget(const Dimensions& dims, uint32_t updateIntervalMs);

    void initialize(LGFX* lcd, LoggerInterface& logger) override;
    void drawStatic() override;
    void cleanUp() override;
    void setUpdateInterval(uint32_t intervalMs) override;
    bool needsUpdate() const override;
    Dimensions getDimensions() const override;

   protected:
    LGFX* lcd_ = nullptr;
    LoggerInterface* logger_ = nullptr;
    Dimensions dimensions_;
    uint32_t updateIntervalMs_;
    uint32_t lastUpdateTimeMs_ = 0;
    bool initialized_ = false;
    bool staticDrawn_ = false;
};