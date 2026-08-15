#pragma once

#include "Colors.h"
#include "config/LgfxConfig.h"
#include "utils/LoggerInterface.h"
#include "utils/ScreenLogQueue.h"

class DisplayContext {
 public:
    DisplayContext(LGFX& display, Colors& colors, LoggerInterface& logger,
                   ScreenLogQueue& screenLogQueue)
        : display_(display), colors_(colors), logger_(logger), screenLogQueue_(screenLogQueue) {}

    DisplayContext(const DisplayContext&) = delete;
    DisplayContext& operator=(const DisplayContext&) = delete;

    LGFX& getDisplay() { return display_; }

    Colors& getColors() { return colors_; }

    LoggerInterface& getLogger() { return logger_; }

    // Only BootScreen needs this — threaded through here (rather than
    // through LoggerInterface itself) so it doesn't have to depend on the
    // full log-sink interface just to drain its boot-message queue.
    ScreenLogQueue& getScreenLogQueue() { return screenLogQueue_; }

 private:
    LGFX& display_;
    Colors& colors_;
    LoggerInterface& logger_;
    ScreenLogQueue& screenLogQueue_;
};