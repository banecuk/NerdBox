#pragma once

#include <cstdint>

// Design-system tokens: surfaces, spacing, and radii shared across widgets so
// visual decisions live in one place instead of at each call site (see
// docs-local/03-visual-ux.md §4-5, V1). Same shape as Layout.h — a plain
// namespace of constexpr values, no class/interface indirection, since every
// value is known at compile time.
//
// kGround stays equal to TFT_BLACK until every widget that currently paints
// TFT_BLACK-as-background has been migrated to take a surface colour as a
// parameter (see docs-local/03-visual-ux.md V1's sequencing note) — flipping
// it before that lands would just repaint everything a slightly-off black
// with no cards to justify it. Widgets adopt these tokens incrementally.
namespace Theme {

// Surfaces — a three-step dark ramp, not pure black. Pure #000 on an ST7796
// reads as a hole; a near-black ground with genuinely raised cards is what
// separates "modern dark UI" from "unlit".
constexpr uint16_t kGround = 0x0000;   // page background (== TFT_BLACK until V6 migrates widgets)
constexpr uint16_t kSurface = 0x18E3;  // raised card
constexpr uint16_t kSurface2 = 0x2124; // nested/sunken track
constexpr uint16_t kOutline = 0x3186;  // 1px card edge (brighter than Colors::kBorderGrey)

// Spacing — 4px base unit. Every gap/gutter/inset is a multiple.
constexpr uint8_t kUnit = 4;
constexpr uint8_t kGutter = 3 * kUnit;  // 12 — matches SettingsScreen today
constexpr uint8_t kGap = 2 * kUnit;     // 8

// Radius ladder.
constexpr uint8_t kRadiusSm = 4;   // pills, chips, small tiles
constexpr uint8_t kRadiusMd = 8;   // cards, buttons
constexpr uint8_t kRadiusLg = 12;  // full-bleed panels

}  // namespace Theme
