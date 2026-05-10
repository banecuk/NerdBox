# PC Metrics Display

Real-time PC performance monitoring on a **WT32-SC01-PLUS** (ESP32) touchscreen device. Metrics are fetched from a PC running [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) via its JSON API and rendered at up to 30 fps on the 480×320 ST7796 display.

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
| CPU | Total load %, per-thread load bars, power (W), temperature (°C) |
| GPU | 3D load %, Compute load %, VRAM usage % |
| RAM | Memory load % |
| Motherboard | CPU fan, front/back case fan RPM |
| Clock | Real-time HH:MM:SS from NTP, falls back to uptime |
| Web UI | `/`, `/app-info`, `/system-info`, screen switch endpoints |

---

## Hardware requirements

| Part | Notes |
|---|---|
| WT32-SC01-PLUS | ESP32-S3 + 3.5″ ST7796 display + FT5x06 capacitive touch |
| USB-C cable | For flashing and serial monitoring |
| PC running Libre Hardware Monitor | Windows only; web server must be enabled |
| 2.4 GHz Wi-Fi | ESP32 does not support 5 GHz |

---

## Software requirements

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- Arduino framework with ESP32-S3 support (installed automatically by PlatformIO)
- [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) ≥ 0.9 on the monitored PC

PlatformIO automatically installs these libraries (declared in `platformio.ini`):

| Library | Purpose |
|---|---|
| `lovyan03/LovyanGFX ^1.2.7` | Display driver and touch input |
| `bblanchon/ArduinoJson ^7.4.1` | JSON parsing with filter support |

---

## Quick start

### 1. Enable Libre Hardware Monitor web server

1. Launch Libre Hardware Monitor on your PC.
2. Go to **Options → Web Server → Run Web Server** and enable it.
3. Note the machine's local IP address (e.g. `192.168.1.11`). Verify the API is reachable from a browser: `http://192.168.1.11:8085/data.json`.

### 2. Clone and configure

```bash
git clone <repository-url>
cd pc-metrics-display

cp include/Environment.h.example include/Environment.h
```

Edit `include/Environment.h`:

```cpp
constexpr char WIFI_SSID[]     = "YourNetworkName";
constexpr char WIFI_PASSWORD[] = "YourPassword";
constexpr char LIBRE_HM_API[]  = "http://192.168.1.11:8085/data.json";

// Set this to your GPU's total VRAM in MB (check Device Manager or GPU-Z)
constexpr float GPU_VRAM_MB    = 16368.0f;
```

> `Environment.h` is listed in `.gitignore` and will never be committed. Never commit credentials.

### 3. Build and flash

Connect the WT32-SC01-PLUS via USB, then:

```bash
pio run --target upload       # build + flash
pio device monitor            # open serial monitor at 115200 baud
```

### 4. Verify

The display should show a boot log, then transition to the main screen within ~10 seconds. Serial output should include:

```
[INFO] WiFi connected - IP: 192.168.1.xxx
[INFO] Time synchronized successfully
[INFO] HTTP Server started
Parsing time: 110 ms
```

---

## Configuration reference

All tunable values live in `src/config/AppConfig.h` inside the `AppConfig::internal` namespace. Change them there; the `AppConfigService` exposes them through the `AppConfigInterface` so no other files need editing.

| Struct | Constant | Default | Description |
|---|---|---|---|
| `DebugImpl` | `kSerialBaudRate` | `115200` | Serial monitor baud rate |
| `DebugImpl` | `kWaitForSerial` | `false` | Block setup until serial port opens (useful for development) |
| `HardwareMonitorImpl` | `kRefreshMs` | `500` | Polling interval when data is healthy |
| `HardwareMonitorImpl` | `kRefreshAfterFailureMs` | `3000` | Back-off interval after a failed fetch |
| `HardwareMonitorImpl` | `kMaxRetries` | `2` | Consecutive failures before a warning is logged |
| `PcMetricsImpl` | `kCores` | `18` | Total logical CPU threads to read. **Must match your CPU.** |
| `UiImpl` | `kTransitionTimeoutMs` | `1000` | Maximum time allowed for a screen transition |
| `UiImpl` | `kTouchDebounceIntervalMs` | `200` | Minimum ms between registered touch events |
| `TimingImpl` | `kScreenTaskMs` | `33` | Target frame period (~30 fps) |
| `TimingImpl` | `kBackgroundTaskMs` | `20` | Background task tick period |
| `WatchdogImpl` | `kTimeoutMs` | `20000` | Hardware watchdog timeout in ms |

### Adjusting for your CPU

`kCores` must equal the number of logical processors reported by Libre Hardware Monitor. For a 12-core/24-thread CPU set it to `24`. Mismatches cause `Insufficient CPU load entries` warnings and missing thread bars.

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
| `PcMetricsService` | `services/pcMetrics/PcMetricsService.h` | Fetches and parses Libre Hardware Monitor JSON |
| `UiController` | `ui/UIController.h` | Screen lifecycle, transition state machine, touch routing |
| `WidgetManager` | `ui/WidgetManager.h` | Owns and updates all widgets on the active screen |
| `EventBus` | `core/events/EventBus.h` | Publish/subscribe for UI actions (brightness, reset, screen change) |

### FreeRTOS tasks

| Task | Core | Period | Purpose |
|---|---|---|---|
| `ScreenUpdate` | 1 (Arduino core) | 33 ms | Calls `UiController::updateDisplay()` |
| `BackgroundTask` | 0 | 20 ms | Polls PC metrics; feeds watchdog |
| Main loop | 1 | 10 ms | Processes web server requests |

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

### Sensor parsing pipeline

```
HTTP GET /data.json
       │
       ▼ (filtered deserializeJson — only Text/Value fields retained)
  JsonDocument
       │
       ▼
 findHardwareIndices()  →  locate Motherboard / CPU / Memory / GPU by Text
       │
       ├── MotherboardParser  →  cpu_temperature, cpu_fan, front_fan, back_fan
       ├── CpuParser          →  cpu_load, cpu_thread_load[], cpu_power
       ├── MemoryParser       →  mem_load
       └── GpuParser          →  gpu_3d, gpu_compute, gpu_mem
```

---

## Adding a new widget

1. Create `src/ui/widgets/MyWidget.h` and `.cpp` extending `Widget`.
2. Implement the three required methods:

```cpp
class MyWidget : public Widget {
public:
    MyWidget(DisplayContext& ctx, const Dimensions& dims, uint32_t updateIntervalMs);
    void drawStatic() override;          // called once on screen enter
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
    return std::make_unique<MyScreen>(logger, display, controller, config);
```

4. Navigate to it via `EventBus` or `UiController::requestScreen(ScreenName::MY_SCREEN)`.

---

## Troubleshooting

### No metrics / "HM update failed"

- Confirm Libre Hardware Monitor is running and **Options → Web Server → Run Web Server** is enabled.
- Open `http://<PC-IP>:8085/data.json` in a browser from the PC. Then try from another device on the same network.
- Check that `LIBRE_HM_API` in `Environment.h` matches exactly (IP address and port).
- Inspect serial output for `JSON deserialization failed` — this usually means the filter depth is wrong for your hardware's JSON structure.
- Verify `kCores` in `AppConfig.h` matches your CPU's logical thread count.

### "Insufficient CPU load entries" warning

Your CPU has a different number of logical threads than `kCores`. Count the thread entries under your CPU in Libre Hardware Monitor's Load section and set `kCores` to that number.

### GPU memory percentage looks wrong

`GPU_VRAM_MB` in `Environment.h` is hard-coded to your GPU's capacity. Update it to your card's actual VRAM (e.g. `8192.0f` for 8 GB).

### Display shows nothing / freezes on boot

- Check serial output at 115200 baud immediately after power-on.
- `kWaitForSerial = true` in `AppConfig.h` will block until a serial monitor is attached — useful for diagnosing early boot failures.
- If the watchdog triggers, the reset reason and WDT status are printed on the next boot.

### WiFi never connects

- The ESP32 only supports 2.4 GHz. Confirm your SSID is on the 2.4 GHz band.
- Increase `kDefaultNetworkRetries` and `kNetworkRetryDelayMs` in `AppConfig.h` for slower routers.

### Web UI not accessible

The web server only starts when WiFi is connected. Check serial for `HTTP Server started`. The device's IP address is printed on successful connection.

---

## Known limitations

- **Single network**: credentials are compiled in. There is no runtime WiFi configuration UI.
- **No TLS**: the Libre Hardware Monitor API is accessed over plain HTTP. Do not expose port 8085 outside your local network.
- **Sensor path coupling**: sensor names (e.g. `"Intel Core"`, `"AMD Radeon"`) are matched by substring in `PcMetricsService::findHardwareIndices()`. Hardware with unusual names may not be detected. Edit the match strings in that function to suit your hardware.
- **GPU VRAM**: the percentage is computed from a hardcoded capacity constant. See [Configuration reference](#configuration-reference).
- **No OTA**: firmware updates require a USB connection.

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

- [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) — sensor data API
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — display and touch driver
- [ArduinoJson](https://arduinojson.org/) — fast filtered JSON parsing
- [PlatformIO](https://platformio.org/) — build system