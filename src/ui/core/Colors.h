#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

// static const byte COLOR_LEVELS_COUNT = 17;
// static const uint16_t COLOR_LEVELS[COLOR_LEVELS_COUNT] = { 0x09ea, 0x1229, 0x22c6,
// 0x3363, 0x3ba2, 0x4400, 0x5c40, 0x7460, 0x8ca0, 0x9CC0, 0xa460, 0xb400, 0xbba0, 0xCB20,
// 0xd280, 0xe180, 0xF800 };

class Colors {
 public:
    // One {breakpoint, color} pair in a ramp sampled by sampleRamp(). Stops
    // must be sorted ascending by `at`; sampleRamp() linearly interpolates
    // between consecutive stops and clamps outside the first/last.
    struct GradientStop {
        uint8_t at;
        uint16_t color;
    };

 private:
    // Static, not per-instance: Colors is only ever constructed once (as a
    // member of ApplicationComponents), so there's no reason for these 800
    // bytes of lookup tables to live inside every Colors object — keeping
    // them static keeps sizeof(Colors) (and therefore ApplicationComponents)
    // unchanged from before these tables existed.
    static uint16_t COLOR_GRADIENT[100];
    static uint16_t COLOR_GRADIENT_DIM[100];
    static uint16_t COLOR_GRADIENT_GPU[100];
    static uint16_t COLOR_GRADIENT_RAM[100];
    // Light-gray-to-light-green ramp shared by every widget that wants a
    // smooth "idle/low -> good/high" scale instead of a metric-specific hue
    // (CpuClockWidget's per-core MHz color, NetworkTrafficWidget's idle-to-
    // moderate utilisation color) — named for the ramp's colors, not either
    // caller's metric, since it isn't specific to either one.
    static uint16_t COLOR_GRADIENT_GRAY_GREEN[100];
    // Blue-to-purple counterpart to COLOR_GRADIENT, used to tint hybrid-CPU
    // E-core bars in ThreadsWidget so they're distinguishable at a glance from
    // P-core bars at the same load — same breakpoints. Stays clearly blue
    // through the low/mid stops; only the top (99%) stop turns purple. See
    // docs-local/13-threads-ecore-distinction.md.
    static uint16_t COLOR_GRADIENT_ECORE[100];
    static void generateGradient();
    static uint16_t diskActivityColorScale(float kbPerSec, uint16_t darkColor,
                                            uint16_t brightColor);

 public:
    // Shared chrome constants — hairline borders/separators/idle states, and
    // inactive-label text, used across widgets so there's one place to tune
    // the look instead of scattered literals.
    static constexpr uint16_t kHairline = 0x2104;      // dark grey
    static constexpr uint16_t kInactiveText = 0x6B4D;  // mid-grey

    // Shared accent/chrome tokens previously duplicated as raw RGB565
    // literals (with "same shade as X" comments) across several widgets —
    // see docs-local/12-code-architecture.md, C9.
    static constexpr uint16_t kCpuAccent = 0xC618;     // near-white CPU accent
    static constexpr uint16_t kGpuAccent = 0xB471;     // muted desaturated-red GPU accent
    static constexpr uint16_t kBorderGrey = 0x2965;    // very dark grey border/track/separator
    static constexpr uint16_t kDimLabelGrey = 0x8410;  // dim grey unit/label text
    static constexpr uint16_t kAmberAccent = 0xFD20;   // amber warning/moderate accent
    static constexpr uint16_t kMutedBlueGrey = 0x4208;  // dim blue-grey, subordinate chrome
    static constexpr uint16_t kRamAccent = 0xADFB;      // RAM tile/sparkline accent (was N12 dup)
    static constexpr uint16_t kWeekendRed = 0xFBCF;     // SAT/SUN light-red tint (was N12 dup)

    // Semantic ramp, ~80% saturation — replaces the saturated 1990s-VGA
    // TFT_RED/TFT_GREEN/TFT_YELLOW/TFT_CYAN primaries that were scattered
    // across widgets as ad hoc "success/warning/danger/info" colours (see
    // docs-local/03-visual-ux.md V2). Distinct from kAmberAccent, which stays
    // as its own tuned amber for the specific spots that already used it.
    static constexpr uint16_t kOk = 0x4E2D;      // muted green   (~76,196,104)
    static constexpr uint16_t kWarn = 0xEE27;    // muted gold    (~235,199,56)
    static constexpr uint16_t kDanger = 0xD9A8;  // muted red     (~220,52,64)
    static constexpr uint16_t kInfo = 0x4D5E;    // muted sky blue(~76,168,248)
    // Muted orange, the midpoint between kWarn and kDanger — the WiFi/signal
    // "degraded" tier shared by NetworkWidget/WifiLinkWidget/WifiScanListWidget
    // (see WifiScanMath::SignalTier, docs-local/13-wifi-screen-plan.md §4.2).
    static constexpr uint16_t kDegraded = 0xE3E8;

    Colors();
    ~Colors();

    // Alpha-weighted blend between two RGB565 colors (alpha=0 -> a, alpha=255 -> b).
    static uint16_t blendRgb565(uint16_t a, uint16_t b, uint8_t alpha);

    // Samples a piecewise-linear ramp defined by `stops` (ascending `at`,
    // n >= 2) at `value`, blending between the two bracketing stops.
    // Host-testable — no LGFX/Colors-instance dependency.
    static uint16_t sampleRamp(const GradientStop* stops, size_t n, uint8_t value);

    // Disk activity color scales, in KB/s: <1 MB/s dark gray (idle), then a
    // continuous blend from a dark to a bright shade of the scale's hue as
    // the rate climbs to a 100 MB/s cap (everything at/above stays at the
    // brightest shade). Read uses green shades, write uses red shades, so the
    // two directions stay visually distinguishable at a glance.
    static uint16_t diskReadActivityColor(float kbPerSec);
    static uint16_t diskWriteActivityColor(float kbPerSec);

    // Reached through an instance (colors_.getColorFromPercent(...)) even
    // though they're static — the lookup tables they index are static too
    // (see the comment above), so there's no per-instance state to dispatch
    // on. Kept callable via an instance rather than renaming every call site.
    static uint16_t getColorFromPercent(uint8_t value, bool dim = false);
    // E-core counterpart to getColorFromPercent(): same 0/25/60/99
    // breakpoints, but a blue-to-purple hue (blue through the low/mid range,
    // purple only at 99%) instead of blue-to-red, so a hybrid CPU's two core
    // classes are distinguishable at a glance in ThreadsWidget. See
    // docs-local/13-threads-ecore-distinction.md.
    static uint16_t getColorFromPercentEcore(uint8_t value);
    static uint16_t getColorFromPercentGpu(uint8_t value);
    static uint16_t getColorFromPercentRam(uint8_t value);
    // value is 0-99, mapping linearly onto whatever range the caller scaled
    // its raw metric into (see CpuClockWidget::clockPercent,
    // NetworkTrafficWidget::trafficColor) — the gradient itself doesn't know
    // what metric it's for, same as the other getColorFromPercent*
    // accessors; it just indexes a precomputed table instead of blending two
    // RGB565 colors on every draw.
    static uint16_t getColorFromPercentGrayGreen(uint8_t value);
    // Four-step utilisation ladder shared by any widget colouring a rate
    // against a configured cap (percent of that cap): a light-grey-to-
    // light-green ramp for 0-60%, yellow for 60-85%, orange for 85-100%, a
    // lightened red at/over 100%. Extracted so NetworkTrafficWidget's
    // per-direction colour and DiskSummaryWidget's write colour can't drift
    // apart on the thresholds (see docs-local/12-code-architecture.md, C2).
    static uint16_t utilizationColor(float percent);
    static uint16_t darken(uint16_t color, uint8_t alpha);
};