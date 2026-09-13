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

// Bottom band shared by every screen: the back/settings button and the clock
// beside it. Previously 272/48 with MainScreen's equivalent band hardcoded
// 3px higher (269) to make room for NetworkWidget/NetworkTrafficWidget
// sharing the row — that left a 3px dead strip at the very bottom of the
// screen (317..320) and put MainScreen's band 3px out of line with every
// other screen's (N1). Moved to 269/51 so one band position/height serves
// every screen: MainScreen's band already sat at 269, so this folds its
// local duplicate into the shared constant instead of the other way round —
// see docs-local/03-visual-ux.md V3.
constexpr uint16_t kBottomBarY = 269;
constexpr uint16_t kBottomBarH = 51;

// Bottom-left back/settings button — square, same size on every screen.
constexpr uint16_t kButtonSize = 51;

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
