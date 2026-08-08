---
name: run-nerdbox
description: Build, flash, run, and drive NerdBox — the ESP32-S3 firmware for the WT32-SC01-PLUS device. Use when asked to build NerdBox, flash it to the device, run its native unit tests, check device status, switch its screen, pull live PC-metrics, or otherwise interact with a running NerdBox device over its HTTP API.
---

NerdBox is embedded firmware, not a GUI/CLI process you spawn locally — there
is no way to run it without the physical WT32-SC01-PLUS device attached over
USB. Once flashed, it joins Wi-Fi and drives itself over `.claude/skills/run-nerdbox/driver.ps1`
(PowerShell — the project is Windows-only: COM ports, `esptool`), which builds
via PlatformIO, flashes, discovers the device's IP, and talks to its HTTP API
(`/api/status`, `/screen/*`, `/api/pc`, …). The one thing this cannot do is
photograph the physical LCD — "driving" it means confirming state changes
through the API (e.g. `/api/status`'s `ui.screen` field), not a screenshot.

All paths below are relative to the repo root (`NerdBox/`).

## Prerequisites

- PlatformIO Core, installed at `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`
  (or on `PATH` as `pio`). If missing: `pip install platformio`.
- The WT32-SC01-PLUS plugged in over USB. It shows up in `pio device list` as
  a USB Serial Device with hardware ID `VID:PID=303A:80D0` (Espressif's vendor
  ID) — that's how the driver's `ip` command finds it (see Gotchas).
- `src/config/Environment.h` populated with real Wi-Fi credentials (copy from
  `Environment.h.example` if it doesn't exist — it's gitignored). Without
  Wi-Fi the device boots but never gets an IP, so the HTTP driver commands
  can't reach it.
- Both device and this machine on the same Wi-Fi subnet (the driver's `ip`
  command reads the Windows ARP table, which only has entries for hosts on
  the local subnet).

## Build

```powershell
.claude\skills\run-nerdbox\driver.ps1 build
```

Builds both `WT32-SC01-PLUS-debug` and `WT32-SC01-PLUS-release`. No hardware
needed for this step.

## Run (agent path)

```powershell
.claude\skills\run-nerdbox\driver.ps1 flash    # build + upload debug firmware
.claude\skills\run-nerdbox\driver.ps1 ip       # discover the device's IP via ARP
.claude\skills\run-nerdbox\driver.ps1 status   # full JSON status snapshot
```

`flash` overwrites whatever is currently on the device — confirm that's
wanted before running it against a device that isn't yours to reflash.

The device re-enumerates its USB serial port a few times around a reset
(bootloader mode → native USB CDC), so don't chain a serial `monitor` call
right after `flash` expecting boot-log text — it's a race you'll usually
lose. Use the HTTP driver commands instead; they don't care about serial
timing.

| command | what it does |
|---|---|
| `ip [-Mac xx:xx:xx:xx:xx:xx] [-DeviceIp x.x.x.x]` | Resolves the device's LAN IP. Auto-detects the connected board's MAC via `pio device list` and looks it up in the ARP table; pass `-DeviceIp` directly to skip discovery once you know it. |
| `status` | `GET /api/status` as JSON — Wi-Fi RSSI, heap, PC-metrics feed state (`pc`/`pc_stream`), current screen (`ui.screen`), task stack high-water marks. |
| `pc` | `GET /api/pc` — full current `PcMetrics` snapshot (CPU/GPU/RAM/fans/disks). |
| `config` | `GET /config` — active `AppSettings` tuning constants + build mode. |
| `logs` | `GET /logs` — recent log entries. |
| `screen <main\|settings\|game\|weather>` | `POST /screen/<name>`, then re-polls `/api/status` and prints the confirmed `ui.screen` — this is how you verify a screen switch actually landed. |
| `restart` | `POST /restart` — reboots the device (verified: uptime resets, Wi-Fi/PC-stream reconnect within ~5s). |
| `test` | `pio test -e native` — host-side unit tests, no hardware needed. |
| `monitor [-Port COM5]` | Attaches `pio device monitor` (blocking; Ctrl-C to quit). Only useful once the port has settled post-boot — see Gotchas. |

Every command accepts `-DeviceIp <ip>` to skip MAC/ARP discovery if you
already know the address.

## Run (human path)

```powershell
pio run -e WT32-SC01-PLUS-debug --target upload   # flash
pio device monitor                                 # serial log at 115200 baud
```

Same as the agent path's `flash`/`monitor`, run directly. The web UI at
`http://<device-ip>/` is the human equivalent of the driver's HTTP commands.

## Test

```powershell
.claude\skills\run-nerdbox\driver.ps1 test
```

32 test cases across `ValueSmootherTest` and `SseEventParserTest`, all pure
host-side logic (`[env:native]`) — no ESP32 hardware involved. Confirmed
passing.

## Gotchas

- **The board's "serial number" is its Wi-Fi MAC.** `pio device list` reports
  `SER=3C:84:27:13:A7:3C`-style values for connected devices; on this board
  (native USB CDC) that string is literally the station MAC address, so it
  doubles as the ARP lookup key. The driver's `ip` command depends on this —
  if PlatformIO ever reports a different serial-number format, `ip` breaks
  and you'd need `-DeviceIp` instead.
- **The upload port and the running-app port can differ transiently.**
  `esptool` puts the board into a bootloader mode that briefly enumerates
  differently (observed as `COM4` mid-upload even though `-upload-port COM5`
  was passed) before the app boots and it settles back to its normal port. A
  serial monitor attached in that gap gets `FileNotFoundError` opening the
  port, or attaches too late and misses the boot log entirely. The HTTP
  driver commands don't have this problem — poll `status` instead of trying
  to catch boot text over serial.
- **`platformio.ini`'s `monitor_port = COM[4]` regex doesn't match reality**
  on this specific device — it enumerates as `COM5` once booted. Pass
  `-Port COM5` (or whatever `pio device list` currently shows) explicitly
  rather than trusting the ini default.
- **Environment.h secrets never belong in this skill or its output.** It
  holds real Wi-Fi credentials and API keys — treat it like any other
  gitignored secret file; don't echo its contents into logs, commits, or
  driver output.

## Troubleshooting

- **`No USB-serial device found (pio device list showed no SER=... entry)`**
  (from the driver's `ip` command): board isn't plugged in, or Windows
  hasn't finished enumerating it yet — re-run `pio device list` manually
  first.
- **`No ARP entry for MAC ...`** (from `ip`): the device hasn't joined Wi-Fi
  yet (check `Environment.h` has real credentials, not the `.example`
  placeholders), or your machine and the device are on different subnets/
  VLANs so no ARP entry exists. `Invoke-RestMethod` calls will also fail
  until this resolves — use `-DeviceIp` once you find the address another
  way (router's DHCP client list, etc.).
- **`could not open port 'COMx': FileNotFoundError`** from `monitor`: hit
  during the boot-time port-renumbering window described in Gotchas. Wait
  a few seconds after flashing and re-check `pio device list` before
  retrying, or just use the HTTP driver instead.
