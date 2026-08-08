#pragma once

#include <cstdint>

#include "config/LgfxConfig.h"

// Shared screen-layout constants. Every widget screen runs the panel rotated
// to landscape, so the app's width/height are the panel's TFT_HEIGHT/
// TFT_WIDTH (not the other way around) — derive from LgfxConfig.h rather
// than adding another hardcoded 480/320 pair.
namespace Layout {

constexpr uint16_t kScreenW = TFT_HEIGHT;
constexpr uint16_t kScreenH = TFT_WIDTH;

// Bottom band shared by GameScreen/DiskScreen/WeatherScreen/SettingsScreen:
// the back/settings button and the clock beside it. MainScreen's equivalent
// band sits 3px higher (y=269) to make room for NetworkWidget/
// NetworkTrafficWidget sharing the row — that offset stays local to
// MainScreen rather than being folded in here.
constexpr uint16_t kBottomBarY = 272;
constexpr uint16_t kBottomBarH = 48;

// Bottom-left back/settings button — square, same size on every screen.
constexpr uint16_t kButtonSize = 48;

// Clock widget width — shared everywhere it appears; height and position
// vary slightly per screen (row height, gutters) and stay local to each.
constexpr uint16_t kClockW = 150;

}  // namespace Layout
