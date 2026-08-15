#pragma once

#include <cstdint>

enum class ScreenName : uint8_t {
    NONE,
    BOOT,
    MAIN,
    SETTINGS,
    GAME,
    DISKS,
    WEATHER,
    CALENDAR
};