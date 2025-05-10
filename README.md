# NerdBox WT32-SC01-PLUS

## Overview
NerdBox is an ESP32-based desktop device built on the WT32-SC01-PLUS, featuring a touchscreen display for showing PC metrics (CPU/GPU load), time, and date. It includes a web server for system information and screen control.

## Hardware Requirements
- WT32-SC01-PLUS (ESP32-S3 with 320x480 TFT display)
- USB-C cable for power and programming
- WiFi network for data fetching

## Setup Instructions
1. Install [PlatformIO](https://platformio.org/).
2. Clone this repository.
3. Copy `src/config/Environment.h.example` to `src/config/Environment.h` and fill in your WiFi credentials and API endpoints.
4. Build and upload the project using PlatformIO.

## Usage
- **Touchscreen**: Tap buttons to navigate between Main and Settings screens or adjust brightness.
- **Web Interface**: Access `http://<device-ip>/` for system info or to switch screens.
