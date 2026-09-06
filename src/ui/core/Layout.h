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

// Content area above the bottom band — every screen with a single
// full-width content widget (DiskScreen, CpuClockScreen, WeatherScreen,
// ProcessesScreen, CalendarScreen, GameScreen's FPS tile) sizes it to
// {0, 0, kScreenW, kContentH}.
constexpr uint16_t kContentH = kBottomBarY;

// Standard bottom-band clock — same box (position/size) on every screen
// that uses BaseWidgetScreen::addBottomClock() (everything except
// MainScreen, which shares its band with NetworkWidget/NetworkTrafficWidget
// and derives a narrower box locally, and SettingsScreen, whose clock has
// its own colour/height/position). Derived, not hardcoded, so the two bands
// can't drift 3px apart by accident (see N1).
constexpr uint16_t kClockH = 40;
constexpr uint16_t kClockX = kScreenW - kClockW - 2;
constexpr uint16_t kClockY = kBottomBarY + (kBottomBarH - kClockH) / 2;

}  // namespace Layout
