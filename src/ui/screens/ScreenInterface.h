#pragma once

#include <cstdint>

#include "config/LgfxConfig.h"

class ScreenInterface {
 public:
    virtual ~ScreenInterface() = default;

    virtual void draw() = 0;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void handleTouch(uint16_t x, uint16_t y) {}
};