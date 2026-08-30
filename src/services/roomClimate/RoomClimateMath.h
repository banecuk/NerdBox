#pragma once

#include <cstdint>

// Pure numeric conversions for the room sensor's raw float reading — no
// Arduino/network dependency, split out of RoomClimateService so the
// negative-temperature rounding path (an outdoor-mounted sensor reads below
// freezing) is host-tested under [env:native] — see
// test/RoomClimateMathTest.cpp.
namespace RoomClimateMath {

// Converts a raw float reading to an x10-scaled int16_t, rounding
// half-away-from-zero (26.55 -> 266, -10.5 -> -105) rather than truncating —
// truncation on a negative value would read up to 1 degree too warm.
int16_t toX10(float value);

// Rounds and clamps a raw float percentage to 0..100.
uint8_t clampHumidity(float value);

}  // namespace RoomClimateMath
