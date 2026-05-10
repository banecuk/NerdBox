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
| GPU | 3D load %, Compute load %, VRAM usage %, temperature (°C), power (W), fan RPM |
| RAM | Memory load % |
| Motherboard | CPU fan RPM, up to 10 system fan RPMs |
| Disk | Per-drive free space %, read KB/s, write KB/s |
| Network | Ethernet upload / download throughput |
| Clock | Real-time HH:MM:SS from NTP, falls back to uptime |
| Web UI | `/`, `/app-info`, `/system-info`, `/screen/main`, `/screen/settings` endpoints |

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
| `TasksImpl` | `kBackgroundStack` | `8096` | FreeRTOS background task stack size (bytes) |
| `HardwareMonitorImpl` | `kRefreshMs` | `500` | Polling interval when data is healthy |
| `HardwareMonitorImpl` | `kThreadsRefreshMs` | `16` | Per-thread load refresh interval |
| `HardwareMonitorImpl` | `kRefreshAfterFailureMs` | `3000` | Back-off interval after a failed fetch |
| `HardwareMonitorImpl` | `kMaxRetries` | `2` | Consecutive failures before a warning is logged |
| `HardwareMonitorImpl` | `kThreadsUpwardSmoothing` | `0.4` | EMA alpha for rising thread load |
| `HardwareMonitorImpl` | `kThreadsDownwardSmoothing` | `0.075` | EMA alpha for falling thread load |
| `PcMetricsImpl` | `kCores` | `18` | Total logical CPU threads to read. **Must match your CPU.** |
| `UiImpl` | `kTransitionTimeoutMs` | `1000` | Maximum time allowed for a screen transition |
| `UiImpl` | `kTouchDebounceIntervalMs` | `200` | Minimum ms between registered touch events |
| `UiImpl` | `kDisplayLockTimeoutMs` | `200` | Display mutex acquisition timeout |
| `MetricsImpl` | `kMaxScreenDrawTimes` | `30` | Rolling window size for draw-time averaging |

### Adjusting for your CPU

`kCores` must equal the number of logical processors reported by NerdWinSense. For a 12-core/24-thread CPU set it to `24`. Mismatches cause `Insufficient CPU load entries` warnings and missing thread bars.

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
| `Application` | `core/Application.h` | Top-level owner; holds `ApplicationComponents` |
| `ApplicationComponents` | `core/ApplicationComponents.h` | Constructs and wires all subsystems |
| `InitializationStateMachine` | `core/InitializationStateMachine.h` | Boot sequence: display → tasks → network → NTP → watchdog |
| `TaskManager` | `core/TaskManager.h` | Creates FreeRTOS tasks; screen task on core 1, background on core 0 |
| `EventBus` | `core/events/EventBus.h` | Singleton publish/subscribe for UI actions |
| `PcMetricsService` | `services/pcMetrics/PcMetricsService.h` | Fetches and parses NerdWinSense JSON; tracks data freshness |
| `NtpService` | `services/NtpService.h` | NTP time sync; provides timestamp to Logger |
| `WebServerService` | `services/WebServerService.h` | HTTP server for `/`, `/app-info`, `/system-info`, screen control |
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

### Metrics parsing pipeline

```
HTTP GET /api/v1/system
       │
       ▼ (filtered deserializeJson — only needed fields retained)
  JsonDocument
       │
       ├── CpuParser          →  cpu_load, cpu_thread_load[], cpu_power
       ├── CpuExtendedParser  →  cpu_temperature
       ├── MemoryParser       →  mem_load
       ├── GpuParser          →  gpu_3d, gpu_compute, gpu_mem, gpu_temperature, gpu_power, gpu_fan
       ├── MotherboardParser  →  cpu_fan, system_fans[]
       └── DiskParser         →  diskDrives[] (name, freeSpacePercent, readKBPerSec, writeKBPerSec)
```

---

## Adding a new widget

1. Create `src/ui/widgets/MyWidget.h` and `.cpp` extending `Widget`.
2. Implement the three required methods:

```cpp
class MyWidget : public Widget {
public:
    MyWidget(DisplayContext& ctx, const Dimensions& dims, uint32_t updateIntervalMs);
    void drawStatic() override;           // called once on screen enter
    void draw(bool forceRedraw) override; // called every frame if needsUpdate()
    bool handleTouch(uint16_t x, uint16_t y) override;
};
```

3. Add it to a screen's `createWidgets()`:

```cpp
void MainScreen::createWidgets() {
    widgetManager_.addWidget(std::make_unique<MyWidget>(
        uiController_->getDisplayContext(),
        Dimensions{x, y, width, height},
        updateIntervalMs
    ));
}
```

`WidgetManager` owns the widget's lifetime. `initialize()` is called automatically by `initializeWidgets()` — do not call it manually.

### Widget coordinate system

The display is landscape (480 × 320 px after `setRotation(1)`). Origin is top-left. Widgets are positioned in absolute pixel coordinates — there is no layout engine.

---

## Adding a new screen

1. Add an entry to `ScreenName` in `src/ui/screens/ScreenTypes.h`:

```cpp
enum class ScreenName : uint8_t {
    NONE, BOOT, MAIN, SETTINGS,
    MY_SCREEN   // add here
};
```

2. Create `src/ui/widgetScreens/MyScreen.h/.cpp` extending `BaseWidgetScreen`:

```cpp
class MyScreen : public BaseWidgetScreen {
public:
    MyScreen(LoggerInterface& logger, UiController* ctrl, AppConfigInterface& cfg)
        : BaseWidgetScreen(logger, ctrl, cfg) {}
private:
    void createWidgets() override;
};
```

3. Register it in `ScreenFactory::createScreen()`:

```cpp
case ScreenName::MY_SCREEN:
    return std::make_unique<MyScreen>(logger, controller, config);
```

4. Navigate to it via the `EventBus` or directly:

```cpp
uiController_.requestScreen(ScreenName::MY_SCREEN);
// or
EventBus::getInstance().publish(EventType::SHOW_MY_SCREEN);
```

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

`GPU_VRAM_MB` in `Environment.h` is hard-coded to your GPU's total capacity. Update it to your card's actual VRAM (e.g. `8192.0f` for 8 GB).

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

The web server starts only after WiFi connects. Check serial for `HTTP Server started`. The device IP is printed on successful connection.

---

## Known limitations

- **Single network**: credentials are compiled in. There is no runtime WiFi configuration UI.
- **No TLS**: the NerdWinSense API is accessed over plain HTTP. Do not expose port 8086 outside your local network.
- **GPU VRAM**: percentage is computed from a hardcoded capacity constant in `Environment.h`.
- **No OTA**: firmware updates require a USB connection.
- **AirVisual API**: the `AIR_VISUAL_API` constant is defined in `Environment.h` but the feature is not yet implemented.

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
- [PlatformIO](https://platformio.org/) — build system