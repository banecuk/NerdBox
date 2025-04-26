#pragma once

#include <LovyanGFX.hpp>
#include <cstdint>  // Include for uint16_t

#include "config/LgfxConfig.h"

class IScreen {
   public:
    virtual ~IScreen() = default;  // Virtual destructor for proper cleanup

    virtual void draw() = 0;   // Make draw pure virtual if every screen MUST implement it
    virtual void onEnter() {}  // Optional
    virtual void onExit() {}   // Optional

    // Add touch handling - provide default empty implementation
    virtual void handleTouch(uint16_t x, uint16_t y) {}
};