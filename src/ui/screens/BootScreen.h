#pragma once

#include "ScreenInterface.h"
#include "utils/Logger.h"

class BootScreen : public ScreenInterface {
 public:
    explicit BootScreen(LoggerInterface& logger, LGFX* lcd);
    ~BootScreen() override = default;

    void initialize();
    void onEnter() override;
    void onExit() override;
    void draw() override;

 private:
    LoggerInterface& logger_;
    LGFX* lcd_;
    int lineNumber_ = 0;
};