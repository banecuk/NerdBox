#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class PcMetrics {
 private:
 public:
    bool is_available = false;
    unsigned long last_update_timestamp = 0;

    uint8_t cpu_temperature = 0;
    uint8_t gpu_temperature = 0;

    uint8_t cpu_load = 0;
    uint8_t gpu_load = 0;
    uint8_t mem_load = 0;
    uint8_t cpu_thread_load[24] = {};

    uint16_t cpu_power = 0;
    uint16_t gpu_power = 0;

    uint16_t cpu_fan = 0;
    uint16_t gpu_fan = 0;
    uint16_t system_fans[10] = {};

    uint8_t gpu_3d = 0;
    uint16_t gpu_compute = 0;
    uint16_t gpu_decode = 0;
    uint16_t gpu_mem = 0;

    float eth_up = 0;
    float eth_dn = 0;
};