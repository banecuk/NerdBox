#pragma once

#include <cstdint>

#include "config/LgfxConfig.h"

class ScreenInterface {
 public:
    virtual ~ScreenInterface() = default;  // Virtual destructor for proper cleanup

    virtual void draw() = 0;   // Make draw pure virtual if every screen MUST implement it
    virtual void onEnter() {}  // Optional
    virtual void onExit() {}   // Optional
    virtual void handleTouch(uint16_t x, uint16_t y) {}
};