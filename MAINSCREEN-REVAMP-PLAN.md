# MainScreen Revamp with GameMetricsWidget Grid — Phase-by-Phase Plan

## Goal

Reuse the `GameMetricsWidget` tile grid (CPU/GPU/RAM/VRAM/fan) from `GameScreen` on
`MainScreen`:

- Place it **below** the current `ThreadsWidget`.
- Use it as the **replacement** for the current `PcMetricsWidget`.
- Move the FPS display to the **top-right free corner** of the display.
- Move disk info **down** to make room (kept as a slim strip).
- Move `AirQualityWidget` **down** to make space.
- Resize `MultiWidget` to fit the remaining space at the bottom; it can expand
  **to the right** since `FpsWidget` has moved to the top-right corner.
- Resize `FpsWidget` to use the available top-right corner space.

## Target layout (480×320, nav band `272..320` unchanged)

| Widget | New position | Notes |
|---|---|---|
| `ThreadsWidget` | `{0,0,308,60}` | unchanged, top-left |
| `FpsWidget` | `{308,0,172,60}` | moved from bottom-right to top-right corner, resized (~172×60) |
| `GameMetricsWidget` | `{0,60,480,90}` | replaces `PcMetricsWidget`; sits directly below threads |
| `DiskBandWidget` *(new)* | `{0,150,480,36}` | extracted compact disk strip |
| `AirQualityWidget` | `{0,186,480,44}` | moved down |
| `MultiWidget` | `{0,230,480,42}` | resized to fill remaining bottom space, expanded full-width to 480 |

Heights sum to 272 (60 + 90 + 36 + 44 + 42), ending exactly at the nav band.
This also removes the old overlap (threads drawn over PcMetrics).

---

## Phase 1 — Make GameMetricsWidget position-relative

**Goal:** `GameMetricsWidget` renders correctly at any position by computing tile
coordinates from its own `dimensions_` instead of hardcoded absolute pixels.
No visible change on the game screen.

**Files:** `src/ui/widgets/display/GameMetricsWidget.h`, `.cpp`

**Steps:**
1. In the header, change the tile row constants to offsets from the widget's
   origin: `kRow1 = 0`, `kRow2 = kRowH`, `kRow3 = 2 * kRowH` (all relative),
   and drop the fixed `130` baseline.
2. Keep column constants (already start at `0` — they are relative to `dims_.x`).
3. Add a private helper that translates a relative tile `Dimensions` to
   absolute screen pixels by adding `dims_.x` / `dims_.y`.
4. Apply the helper in `buildFixedWidgets()` and
   `ensureSystemFanWidgetsCreated()` when constructing each `MetricWidget`.
5. Update the header comment that states tile positions are hardcoded absolute.

**Verification:** `pio run -e WT32-SC01-PLUS-debug` compiles; `GameScreen` still
renders the grid at `{0,130,480,90}` unchanged.

**Done when:** Build passes, no layout change on the game screen.

---

## Phase 2 — Extract DiskBandWidget from PcMetricsWidget

**Goal:** Create a standalone, slim disk-band widget that can be placed anywhere
on a screen, carrying over the existing disk rendering and tap-to-open behaviour.

**Files (new):** `src/ui/widgets/display/DiskBandWidget.h`, `.cpp`

**Steps:**
1. Port the disk machinery from `PcMetricsWidget` into the new widget:
   - `diskDriveWidgets_`, `diskWriteLineColor_`, `diskReadLineColor_`,
     `diskFreeSpaceSmoothed_` state.
   - `ensureDiskWidgetsCreated()`, `updateDiskDriveWidgets()`,
     `drawDiskChevron()`, `initAndDrawWidget()`.
   - `PcMetricsDiskLock` snapshot pattern (copy state under lock, render lock-free).
   - `handleTouch()` on the disk band → publish an `EventType` action via a
     callback (mirror `FpsWidget`'s `action_`/`callback_` pattern, default
     `SHOW_DISKS`).
   - `setStaleTimeout()` via `DataFreshnessGuard`.
2. Derive all tile/line positions from the widget's `dimensions_` so it is
   position-independent (the disk band is one row: write line, tile, read line).
3. Remove the disk-specific members/methods from `PcMetricsWidget.h/.cpp`
   (`diskDriveWidgets_`, `ensureDiskWidgetsCreated`, `updateDiskDriveWidgets`,
   `drawDiskChevron`, related vectors/constants, disk `handleTouch` branch).
   The tile grid + system-fan rendering stays.

**Verification:** `pio run -e WT32-SC01-PLUS-debug` compiles.

**Done when:** Build passes; `PcMetricsWidget` no longer renders disk, and
`DiskBandWidget` is ready to be dropped into a screen.

---

## Phase 3 — Rewrite MainScreen layout

**Goal:** The main screen uses the new layout: threads + FPS on top, the
`GameMetricsWidget` grid below threads, disk strip + air quality below that,
and a full-width `MultiWidget` at the bottom of the content area.

**Files:** `src/ui/widgetScreens/MainScreen.h`, `MainScreen.cpp`

**Steps:**
1. `MainScreen.h`:
   - Remove `#include "ui/widgets/display/PcMetricsWidget.h"`.
   - Add `#include "ui/widgets/display/GameMetricsWidget.h"`,
     `#include "ui/widgets/display/ThreadsWidget.h"`,
     `#include "ui/widgets/display/DiskBandWidget.h"`.
2. `MainScreen.cpp` — rewrite `createWidgets()` in z-order:
   - `ThreadsWidget` `{0,0,308,60}` (unchanged).
   - `FpsWidget` `{308,0,172,60}`, keep `SHOW_GAME` tap.
   - `GameMetricsWidget` `{0,60,480,90}` + `setStaleTimeout(5000)`.
   - `DiskBandWidget` `{0,150,480,36}` with `SHOW_DISKS` action + callback.
   - `AirQualityWidget` `{0,186,480,44}`.
   - `MultiWidget` `{0,230,480,42}` (full width).
   - Bottom nav band (`ButtonWidget`, `NetworkTrafficWidget`, `NetworkWidget`,
     `ClockWidget`) unchanged.
3. Constructor signature and member deps (`pcMetrics_`, `systemMetrics_`,
   `airQualityData_`, `netStatus_`) stay the same.

**Verification:** `pio run -e WT32-SC01-PLUS-debug` compiles; flash + visually
inspect on device (all tiles present, no overlaps, FPS tappable to game screen,
disk strip tappable to disk screen).

**Done when:** Build passes and the on-device layout matches the target table.

---

## Phase 4 — Delete PcMetricsWidget

**Goal:** Remove the now-unused class and its files.

**Files (delete):** `src/ui/widgets/display/PcMetricsWidget.h`, `.cpp`

**Steps:**
1. Grep for remaining `PcMetricsWidget` references; the only code references are
   in `MainScreen.h/.cpp`, both already removed in Phase 3. Everything else is
   comments (leave comment-only references, or optionally tidy them).
2. Delete the two files.

**Verification:** `pio run -e WT32-SC01-PLUS-debug` compiles clean.

**Done when:** Build passes with the files deleted.

---

## Phase 5 — Final verification & polish

**Steps:**
1. `clang-format -i` on every touched file.
2. `pio run -e WT32-SC01-PLUS-debug` (compile check).
3. `pio test -e native` (host-side unit tests still pass).
4. `pio run -e WT32-SC01-PLUS-release` (release build also compiles).
5. Flash to device and confirm the layout visually; tune tile/band heights if
   the disk strip (36px) feels cramped vs the old 35px band.

**Done when:** All builds/tests green and on-device layout confirmed.

---

## Notes / assumptions

- `FpsWidget` needs **no code change** — it renders a label + centered mono
  value and adapts to its dims; a 172×60 tile stays tappable for `SHOW_GAME`.
- `GameMetricsWidget` gets `setStaleTimeout(5000)` like the old widget.
- Disk strip height (32–36) may be tighter than the old band (35px); keep the
  write/read activity-line offsets compact and the tile rows at ~30px within
  the strip.