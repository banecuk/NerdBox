#pragma once

#include <cstdint>

// Per-tile tuning data for PcMetricsWidget's fixed CPU/GPU/RAM/fan tile grid
// — thresholds, colors, and labels, which are the values a user actually
// retunes per machine. Not routed through AppSettings (like AppConfig's
// scalar knobs): this is a data table consumed directly by PcMetricsWidget,
// the same way LgfxConfig.h is consumed directly by the display hardware
// layer. Layout (screen position) and the PcMetrics field each tile reads
// stay in PcMetricsWidget.cpp since they're structural, not data.
//
// Entry order must match the physical tile order PcMetricsWidget.cpp builds
// its dims/getValue arrays in (CPU row, RAM, GPU row, VRAM, 3D/compute) —
// see PcMetricsWidget::fixedTileDescriptors().
namespace PcMetricsTilesConfig {

struct TileData {
    const char* unit;
    int rangeMin;
    int rangeMax;
    float thresholdLow;
    float thresholdHigh;
    const char* label;
    uint16_t labelColor;
    bool useGpuColors;
    bool useDimColors;
    bool useRamColors;
};

inline constexpr const char* kDegreesC =
    "\xC2\xB0"
    "C";

inline constexpr TileData kTiles[] = {
    // CPU row
    {"%", 0, 100, 10.0f, 90.0f, "CPU", 0xC618, false, false, false},
    {kDegreesC, 0, 100, 55.0f, 85.0f, "TMP", 0xC618, false, false, false},
    {" W", 0, 400, 55.0f, 140.0f, "PWR", 0xC618, false, false, false},
    {"", 0, 1500, 800.0f, 1200.0f, "FAN", 0xC618, false, false, false},

    // RAM — end of CPU row
    {"%", 0, 100, 60.0f, 90.0f, "RAM", 0xADFB, false, false, true},

    // GPU row
    {"%", 0, 100, 10.0f, 90.0f, "GPU", 0xB471, true, false, false},
    {kDegreesC, 0, 100, 55.0f, 85.0f, "TMP", 0xB471, true, false, false},
    {" W", 0, 400, 50.0f, 170.0f, "PWR", 0xB471, true, false, false},
    {"", 0, 1500, 800.0f, 1400.0f, "FAN", 0xB471, true, true, false},

    // VRAM — end of GPU row
    {"%", 0, 100, 30.0f, 90.0f, "VRM", 0xB471, true, false, false},

    // Row 3 — 3D / compute (fan slots are lazily created, see
    // PcMetricsWidget::ensureSystemFanWidgetsCreated())
    {"%", 0, 100, 10.0f, 90.0f, "3D", 0xB471, true, false, false},
    {"%", 0, 100, 10.0f, 90.0f, "CMP", 0xB471, true, false, false},
};

}  // namespace PcMetricsTilesConfig
