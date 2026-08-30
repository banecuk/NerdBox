#include "RoomClimateMath.h"

int16_t RoomClimateMath::toX10(float value) {
    const float scaled = value * 10.0f;
    return static_cast<int16_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

uint8_t RoomClimateMath::clampHumidity(float value) {
    const float clamped = value < 0.0f ? 0.0f : (value > 100.0f ? 100.0f : value);
    return static_cast<uint8_t>(clamped + 0.5f);
}
