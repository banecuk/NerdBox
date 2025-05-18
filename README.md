# PC Metrics Display

**Note: This project is a work in progress. Features, functionality, and documentation may change significantly as development continues. Contributions and feedback are welcome to help improve the project.**

This project enables real-time monitoring of PC performance metrics, including CPU load, temperature, RAM usage, GPU load, and more, on a WT32-SC01-PLUS ESP32-based touchscreen device. Metrics are fetched from a PC running **Libre Hardware Monitor**, a fork of **Open Hardware Monitor**, via its JSON API.

## Features
- Displays CPU load, temperature, power, and per-core thread loads.
- Shows GPU load (3D and compute), memory usage, and fan speeds.
- Monitors RAM usage and motherboard sensors (e.g., CPU fan, system fans).
- Periodically updates metrics from the Libre Hardware Monitor API.
- Provides a user-friendly touchscreen interface on the WT32-SC01-PLUS.

## Data Source
PC metrics are sourced from **Libre Hardware Monitor**, a fork of **Open Hardware Monitor**. Libre Hardware Monitor runs on the host PC and exposes sensor data through a JSON API (e.g., `http://192.168.1.11:8085/data.json`). Ensure Libre Hardware Monitor is installed and its web server is enabled for remote access.

## Requirements

### Hardware
- WT32-SC01-PLUS (ESP32 with touchscreen display)
- USB cable for programming and power
- PC running Libre Hardware Monitor

### Software
- [PlatformIO](https://platformio.org/) for building and uploading firmware
- Arduino framework with ESP32 support
- [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) installed on the PC

### Dependencies
Specified in `platformio.ini`:
- `lovyan03/LovyanGFX` (for WT32-SC01-PLUS display)
- `bblanchon/ArduinoJson` (for JSON parsing)

## Setup

1. **Install Libre Hardware Monitor**:
   - Download from the [official repository](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor).
   - Install and run Libre Hardware Monitor on your PC.
   - Enable the web server in settings to expose the JSON API (default: `http://localhost:8085/data.json`).
   - Note the PC's IP address (e.g., `192.168.1.11`) and verify accessibility from the ESP32's network.

2. **Configure the ESP32**:
   - Clone this repository:
     ```bash
     git clone <repository-url>
     ```
   - Open the project in PlatformIO.
   - Copy `Environment.h.example` to `Environment.h`:
     ```bash
     cp include/Environment.h.example include/Environment.h
     ```
   - Edit `Environment.h` to set the API endpoint:
     ```cpp
     #define LIBRE_HM_API "http://192.168.1.11:8085/data.json"
     ```
   - Edit `Environment.h` to set WiFi credentials:  
   - Connect the WT32-SC01-PLUS via USB.

3. **Build and Upload**:
   - Build the project:
     ```bash
     pio run
     ```
   - Upload the firmware to the WT32-SC01-PLUS:
     ```bash
     pio run --target upload
     ```
   - Open the Serial Monitor to verify parsing (baud rate: 115200):
     ```bash
     pio device monitor
     ```

4. **Verify Operation**:
   - The WT32-SC01-PLUS display should show the current time and metrics (e.g., CPU load: ~4%, RAM usage: ~47%, GPU memory: ~13%).
   - Serial output should confirm successful parsing:
     ```
     Parsing time: 110 ms
     ```

## Troubleshooting

- **No metrics displayed**:
  - Verify Libre Hardware Monitor is running and its web server is enabled.
  - Check the IP address and port in `Environment.h`.
  - Inspect Serial Monitor for errors (e.g., `JSON deserialization failed`).
  - Ensure the JSON parsing logic in `PcMetricsService.cpp` matches your hardware's sensor data structure.
- **"HM update failed" in Serial output**:
  - Indicates missing hardware data. Ensure Libre Hardware Monitor reports all sensors (CPU, GPU, RAM).
  - Confirm network connectivity between the ESP32 and PC.
  - Check for memory constraints in Serial output.
  - Adjust JSON parsing logic if your hardware uses different sensor names or structures.

## Contributing

Contributions are welcome! To contribute:
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/your-feature`).
3. Commit changes (`git commit -m "Add your feature"`).
4. Push to the branch (`git push origin feature/your-feature`).
5. Open a pull request.

Please include tests and update documentation as needed.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Acknowledgments

- **Libre Hardware Monitor**: For providing the JSON API for PC metrics.
- **Open Hardware Monitor**: The original project forked by Libre Hardware Monitor.
- **PlatformIO** and **Arduino** communities: For robust development tools and libraries.
- **LovyanGFX**: For WT32-SC01-PLUS display support.