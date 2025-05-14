# PC Metrics Display

This project displays real-time PC performance metrics (CPU load, temperature, RAM usage, GPU load, etc.) on a WT32-SC01-PLUS ESP32-based touchscreen device. The metrics are fetched from a PC running Libre Hardware Monitor, a fork of Open Hardware Monitor, via a JSON API.

## Features
- Displays CPU load, temperature, power, and per-core thread loads.
- Shows GPU load (3D and compute), memory usage, and fan speeds.
- Monitors RAM usage and motherboard sensor data (e.g., CPU fan, system fans).
- Updates metrics periodically from the Libre Hardware Monitor API.
- User-friendly touchscreen interface on the WT32-SC01-PLUS.

## Data Source
The PC metrics are sourced from **Libre Hardware Monitor**, a fork of **Open Hardware Monitor**. Libre Hardware Monitor runs on the host PC and exposes sensor data via a JSON API (e.g., `http://192.168.1.11:8085/data.json`). Ensure Libre Hardware Monitor is installed and configured to enable the web server for remote access.

## Requirements
- **Hardware**:
  - WT32-SC01-PLUS (ESP32 with touchscreen display)
  - USB cable for programming and power
  - PC running Libre Hardware Monitor
- **Software**:
  - PlatformIO for building and uploading the firmware
  - Arduino framework with ESP32 support
  - Libre Hardware Monitor installed on the PC
- **Dependencies** (specified in `platformio.ini`):
  - `lovyan03/LovyanGFX`
  - `bblanchon/ArduinoJson`

## Setup
1. **Install Libre Hardware Monitor**:
   - Download and install Libre Hardware Monitor from its [official repository](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor).
   - Enable the web server in Libre Hardware Monitor settings to expose the JSON API (default: `http://localhost:8085/data.json`).
   - Note the IP address of the PC (e.g., `192.168.1.11`) and ensure it's accessible from the ESP32.

2. **Configure the ESP32**:
   - Clone this repository to your computer.
   - Open the project in PlatformIO.
   - Copy Environment.h.example to Environment.h and update information
   - Ensure the WT32-SC01-PLUS is connected via USB.

3. **Build and Upload:**
    Build the project in PlatformIO: pio run.
    Upload the firmware to the WT32-SC01-PLUS: pio run --target upload.
    Open the Serial Monitor (pio device monitor) to verify parsing (baud rate: 115200).

4. **Verify Operation:**
  - The display should show current time and metrics like CPU load, RAM usage, and GPU memory
