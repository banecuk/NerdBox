# NerdBox

Real-time PC performance monitoring on a **WT32-SC01-PLUS** (ESP32-S3) touchscreen device. Metrics are fetched from a PC running a custom companion app (**NerdWinSense**) via its JSON API and rendered at up to 60 fps on the 480×320 ST7796 display.

> **Status:** Work in progress. API, wire format, and sensor paths may change between versions. Contributions and issue reports are welcome.

---

## Table of contents

- [Features](#features)
- [Hardware requirements](#hardware-requirements)
- [Software requirements](#software-requirements)
- [Quick start](#quick-start)
- [Configuration reference](#configuration-reference)
- [Architecture](#architecture)
- [Adding a new widget](#adding-a-new-widget)
- [Adding a new screen](#adding-a-new-screen)
- [Troubleshooting](#troubleshooting)
- [Known limitations](#known-limitations)
- [Contributing](#contributing)
- [License](#license)

---

## Features

| Category | What is shown |
|---|---|
| CPU | Total load %, per-thread load bars (up to 24 threads), power (W), temperature (°C) |
| GPU | 3D load %, Compute load %, VRAM usage %, temperature (°C), power (W), fan RPM, fullscreen FPS |
| RAM | Memory load % |
| Motherboard | CPU fan RPM, up to 10 system fan RPMs |
| Disk | Per-drive free space %, read KB/s, write KB/s |
| Network | Ethernet upload / download throughput |
| Clock | Real-time HH:MM:SS from NTP, falls back to uptime |
| FPS Widget | GPU fullscreen FPS counter — shown only when a game is running (bottom-right, above clock) |
| IP Widget | Device WiFi IP address on the Settings screen |
| Web UI | `/`, `/app-info`, `/system-info`, `/config`, `/logs`, `/api` (endpoint help), `/api/status`, `/api/raw`, `/api/pc`, `/screen/main`, `/screen/settings`, `/restart` |

---

## Hardware requirements

| Part | Notes |
|---|---|
| WT32-SC01-PLUS | ESP32-S3 + 3.5″ ST7796 display + FT5x06 capacitive touch |
| USB-C cable | For flashing and serial monitoring |
| PC running NerdWinSense | Windows companion app; must be running and reachable on the local network |
| 2.4 GHz Wi-Fi | ESP32 does not support 5 GHz |

---

## Software requirements

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- Arduino framework with ESP32-S3 support (installed automatically by PlatformIO)
- **NerdWinSense** companion app running on the monitored PC (exposes the `/api/v1/system` JSON endpoint)

PlatformIO automatically installs these libraries (declared in `platformio.ini`):

| Library | Purpose |
|---|---|
| `lovyan03/LovyanGFX ^1.2.19` | Display driver and touch input |
| `bblanchon/ArduinoJson ^7.4.3` | JSON parsing with filter support |

---

## Quick start

### 1. Start NerdWinSense on your PC

Launch **NerdWinSense** and confirm it is serving the metrics endpoint. Verify from a browser on the same network:

```
http://<PC-IP>:8086/api/v1/system
```

Note your PC's local IP address (e.g. `192.168.1.11`).

### 2. Clone and configure

```bash
git clone <repository-url>
cd NerdBox

cp src/config/Environment.h.example src/config/Environment.h
```

Edit `src/config/Environment.h`:

```cpp
constexpr char WIFI_SSID[]     = "YourNetworkName";
constexpr char WIFI_PASSWORD[] = "YourPassword";

// NerdWinSense API endpoint
constexpr char LIBRE_HM_API[]  = "http://192.168.1.11:8086/api/v1/system";

// Set to your GPU's total VRAM in MB (check Device Manager or GPU-Z)
constexpr float GPU_VRAM_MB    = 8192.0f;

// Optional: AirVisual API for air quality data (not yet active)
constexpr char AIR_VISUAL_API[] = "http://api.airvisual.com/v2/nearest_city?lat=...&key=YOUR_KEY";
```

> `Environment.h` is listed in `.gitignore` and will never be committed. **Never commit credentials or API keys.**

### 3. Build and flash

Connect the WT32-SC01-PLUS via USB, then:

```bash
pio run --target upload       # build + flash
pio device monitor            # open serial monitor at 115200 baud
```

### 4. Verify

The display shows a boot log, then transitions to the main screen within ~10 seconds. Serial output should include:

```
[INFO] WiFi connected - IP: 192.168.1.xxx
[INFO] Time synchronized successfully
[INFO] HTTP Server started
NerdWinSense JSON Parse Time: 110 ms
```

---

## Configuration reference

All tunable values live in `src/config/AppConfig.h` inside the `AppConfig::internal` namespace. The `AppConfigService` exposes them through `AppConfigInterface` — no other files need editing.

| Struct | Constant | Default | Description |
|---|---|---|---|
| `DebugImpl` | `kSerialBaudRate` | `115200` | Serial monitor baud rate |
| `DebugImpl` | `kSerialTimeoutMs` | `10000` | Serial wait timeout in ms |
| `DebugImpl` | `kWaitForSerial` | `false` | Block setup until serial port opens (useful for development) |
| `InitImpl` | `kDefaultNetworkRetries` | `3` | WiFi connection attempt count |
| `InitImpl` | `kDefaultTimeSyncRetries` | `3` | NTP sync attempt count |
| `InitImpl` | `kNetworkRetryDelayMs` | `200` | Delay between WiFi retries |
| `WatchdogImpl` | `kTimeoutMs` | `20000` | Hardware watchdog timeout in ms |
| `WatchdogImpl` | `kEnableOnBoot` | `true` | Enable watchdog on startup |
| `TimingImpl` | `kScreenTaskMs` | `16` | Target frame period (~60 fps) |
| `TimingImpl` | `kBackgroundTaskMs` | `20` | Background task tick period |
| `TimingImpl` | `kMainLoopMs` | `10` | Main loop tick period |
| `TasksImpl` | `kScreenStack` | `6144` | FreeRTOS screen task stack size (bytes) |
| `TasksImpl` | `kBackgroundStack` | `8192` | FreeRTOS background task stack size (bytes) |
| `HardwareMonitorImpl` | `kRefreshMs` | `500` | Polling interval when data is healthy |
| `HardwareMonitorImpl` | `kThreadsRefreshMs` | `16` | Per-thread widget redraw interval (animation speed, not data freshness) |
| `HardwareMonitorImpl` | `kRefreshAfterFailureMs` | `3000` | Back-off interval after a failed fetch |
| `HardwareMonitorImpl` | `kMaxRetries` | `2` | Consecutive failures before a warning is logged |
| `HardwareMonitorImpl` | `kThreadsUpwardSmoothing` | `0.4` | EMA alpha for rising thread load bars — increase to react faster |
| `HardwareMonitorImpl` | `kThreadsDownwardSmoothing` | `0.075` | EMA alpha for falling thread load bars — **decrease** to decay slower |
| `PcMetricsImpl` | `kCores` | `18` | Total logical CPU threads to read. **Must match your CPU.** |
| `UiImpl` | `kTransitionTimeoutMs` | `1000` | Maximum time allowed for a screen transition |
| `UiImpl` | `kTouchDebounceIntervalMs` | `200` | Minimum ms between registered touch events |
| `UiImpl` | `kDisplayLockTimeoutMs` | `200` | Display mutex acquisition timeout |
| `MetricsImpl` | `kMaxScreenDrawTimes` | `30` | Rolling window size for draw-time averaging |

### Adjusting for your CPU

`kCores` must equal the number of logical processors reported by NerdWinSense. For a 12-core/24-thread CPU set it to `24`. Mismatches cause `Insufficient CPU load entries` warnings and missing thread bars.

### kThreadsRefreshMs vs kRefreshMs

These two constants are **independent**:

- `kRefreshMs` — how often the background task performs an HTTP fetch and writes new data into `PcMetrics`. Reducing this makes data arrive more frequently.
- `kThreadsRefreshMs` — how often the `ThreadsWidget` redraws. The EMA smoother only advances when new data actually arrives, so setting this lower than `kRefreshMs` adds animation smoothness without wasted work.

---

## Architecture

The firmware is split into five layers. Dependencies only flow downward.

```
┌─────────────────────────────────────┐
│              UI layer               │  UiController, Screens, Widgets
├─────────────────────────────────────┤
│           Services layer            │  PcMetricsService, NtpService, WebServerService
├───────────────┬─────────────────────┤
│  Network      │  Core               │  NetworkManager, HttpClient │ TaskManager, EventBus
├───────────────┴─────────────────────┤
│           Config layer              │  AppConfigService, Environment.h
├─────────────────────────────────────┤
│          Hardware layer             │  LGFX, ST7796, FT5x06, ESP32-S3
└─────────────────────────────────────┘
```

### Key classes

| Class | File | Responsibility |
|---|---|---|
| `Application` | `app/Application.h` | Top-level owner; holds `ApplicationComponents` |
| `ApplicationComponents` | `app/ApplicationComponents.h` | Constructs and wires all subsystems |
| `InitializationStateMachine` | `app/InitializationStateMachine.h` | Boot sequence: display → tasks → network → NTP → watchdog |
| `TaskManager` | `app/TaskManager.h` | Creates FreeRTOS tasks; screen task on core 1, background on core 0 |
| `EventBus` | `core/events/EventBus.h` | Singleton publish/subscribe for UI actions |
| `PcMetricsService` | `services/pcMetrics/PcMetricsService.h` | Fetches and parses NerdWinSense JSON; tracks data freshness |
| `NtpService` | `services/NtpService.h` | NTP time sync; provides timestamp to Logger |
| `WebServerService` | `services/web/WebServerService.h` | HTTP server for diagnostics and screen control |
| `UiController` | `ui/core/UiController.h` | Screen lifecycle, transition state machine, touch routing |
| `WidgetManager` | `ui/widgets/layout/WidgetManager.h` | Owns and updates all widgets on the active screen |

### FreeRTOS tasks

| Task | Core | Period | Purpose |
|---|---|---|---|
| `ScreenUpdate` | 1 (Arduino core) | 16 ms | Calls `UiController::updateDisplay()` (~60 fps) |
| `BackgroundTask` | 0 | 20 ms | Polls PC metrics; feeds watchdog |
| Main loop | 1 | 10 ms | Processes web server requests |

### EventBus events

| Event | Trigger |
|---|---|
| `RESET_DEVICE` | Reboot the ESP32 |
| `CYCLE_BRIGHTNESS` | Step through brightness levels |
| `SHOW_SETTINGS` | Navigate to the settings screen |
| `SHOW_MAIN` | Navigate to the main screen |
| `SHOW_ABOUT` | Navigate to the about screen |

### Screen transition lifecycle

```
requestScreen(name)
       │
       ▼
  UNLOADING  →  currentScreen_->onExit(); currentScreen_.reset()
       │
       ▼
  CLEARING   →  display->fillScreen(TFT_BLACK)
       │
       ▼
  ACTIVATING →  ScreenFactory::createScreen(); newScreen->onEnter()
       │
       ▼
    IDLE (draw loop resumes)
```

---

## Adding a new widget

1. Create `src/ui/widgets/display/MyWidget.h` and `.cpp` extending `Widget`.
2. Implement:

```cpp
class MyWidget : public Widget {
public:
    MyWidget(DisplayContext& ctx, const WidgetInterface::Dimensions& dims, uint32_t updateIntervalMs);
    void drawStatic() override;       // called once on screen enter and on forceRedraw
    bool handleTouch(uint16_t x, uint16_t y) override;
protected:
    void onDraw(bool forceRedraw) override;   // called when needsUpdate() or isDirty()
};
```

3. Register in a screen's `createWidgets()`:

```cpp
widgetManager_.addWidget(std::make_unique<MyWidget>(
    uiController_->getDisplayContext(),
    WidgetInterface::Dimensions{x, y, width, height},
    updateIntervalMs
));
```

**Widget memory rules:**
- Never store `String` members. Use `char[]` arrays sized to the maximum expected content.
- Never call `new` or construct `String` objects inside `onDraw`. All drawing must be allocation-free.
- Cache the last drawn value and skip the draw call if nothing changed.

---

## Adding a new screen

1. Add an entry to `ScreenName` in `src/core/ScreenTypes.h`.
2. Create `src/ui/screens/MyScreen.h/.cpp` extending `BaseWidgetScreen` (`src/ui/screens/base/`).
3. Register in `ScreenFactory::createScreen()`. If the screen needs additional dependencies (like `NetworkManager` for `SettingsScreen`), add them to `ScreenFactory::createScreen()`'s signature and thread them through `UiController`.
4. Navigate to it with `uiController_.requestScreen(ScreenName::MY_SCREEN)`.

---

## Troubleshooting

### No metrics / "Failed to fetch data from PC metrics API"

- Confirm NerdWinSense is running on your PC.
- Open `http://<PC-IP>:8086/api/v1/system` in a browser from the PC, then try from another device on the same network.
- Check that `LIBRE_HM_API` in `Environment.h` matches exactly (correct IP and port).
- Inspect serial output for `JSON parsing failed` — the JSON structure from your version of NerdWinSense may differ.
- Verify `kCores` in `AppConfig.h` matches your CPU's logical thread count.

### "Insufficient CPU load entries" warning

Your CPU has a different number of logical threads than `kCores`. Count the thread entries in the NerdWinSense data and set `kCores` to that number.

### GPU memory percentage looks wrong

`GPU_VRAM_MB` in `Environment.h` is hard-coded to your GPU's total capacity. Update it to your card's actual VRAM (e.g. `16384.0f` for 16 GB).

### FPS widget shows nothing even when a game is running

The NerdWinSense API reports `FullscreenFps` as a **float** (e.g. `155.40016`). Ensure `parseGpuData` uses `.as<float>()` before casting to `int16_t` — the `| fallback` operator with an integer fallback silently fails on float-typed JSON nodes.

### Data shown as stale after 5 seconds

`PcMetricsService` marks data stale if no successful fetch has occurred within `DATA_STALE_TIMEOUT_MS` (5 000 ms). Check the network connection and NerdWinSense status.

### Display shows nothing / freezes on boot

- Check serial output at 115200 baud immediately after power-on.
- Set `kWaitForSerial = true` in `AppConfig.h` to block until a serial monitor is attached.
- If the watchdog triggers, the reset reason and WDT status are printed on the next boot.

### WiFi never connects

- The ESP32 only supports 2.4 GHz. Confirm your SSID is on the 2.4 GHz band.
- Increase `kDefaultNetworkRetries` and `kNetworkRetryDelayMs` in `AppConfig.h` for slower routers.

### Web UI not accessible

The web server starts only after WiFi connects. Check serial for `HTTP Server started`. The device IP is printed on connection and is also shown on the Settings screen.

---

## Known limitations

- **Single network**: credentials are compiled in. There is no runtime WiFi configuration UI.
- **No TLS**: the NerdWinSense API is accessed over plain HTTP. Do not expose port 8086 outside your local network.
- **GPU VRAM**: percentage is computed from a hardcoded capacity constant in `Environment.h`.
- **No OTA**: firmware updates require a USB connection.
- **AirVisual API**: the `AIR_VISUAL_API` constant is defined in `Environment.h` but the feature is not yet implemented.
- **No PcMetrics mutex**: `PcMetrics` scalar fields are word-sized and atomically readable on Xtensa, but `diskDrives` is a `std::vector` — concurrent access from the background and screen tasks is not protected.

---

## Contributing

1. Fork the repository and create a feature branch: `git checkout -b feature/your-feature`.
2. Follow the existing code style (enforced by `.clang-format` — run `clang-format -i` before committing).
3. Test on hardware. Include serial output showing successful operation in your PR description.
4. Open a pull request with a clear description of what changes and why.

Issues and feature requests are welcome via GitHub Issues. Please include your hardware (CPU model, GPU model) and the relevant serial output when reporting bugs.

---

## License

MIT — see [`LICENSE`](LICENSE) for details.

---

## Acknowledgments

- [NerdWinSense](https://github.com/) — companion app providing the sensor data API
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — display and touch driver
- [ArduinoJson](https://arduinojson.org/) — fast filtered JSON parsing
