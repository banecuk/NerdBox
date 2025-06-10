#pragma once

#include "Colors.h"
#include "config/LgfxConfig.h"
#include "utils/LoggerInterface.h"

class DisplayContext {
   public:
    DisplayContext(LGFX& display, Colors& colors, LoggerInterface& logger)
        : display_(display), colors_(colors), logger_(logger) {}

    DisplayContext(const DisplayContext&) = delete;
    DisplayContext& operator=(const DisplayContext&) = delete;

    LGFX& getDisplay() { return display_; }
    Colors& getColors() { return colors_; }
    LoggerInterface& getLogger() { return logger_; }

   private:
    LGFX& display_;
    Colors& colors_;
    LoggerInterface& logger_;
};