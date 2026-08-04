#pragma once

#include <cstdint>

enum class EventType : uint8_t {
    NONE = 0,
    RESET_DEVICE,
    CYCLE_BRIGHTNESS,
    SHOW_SETTINGS,
    SHOW_MAIN,
    SHOW_ABOUT,
    SHOW_GAME,
    SHOW_DISKS,
    COUNT  // ← always last; do not use directly
};