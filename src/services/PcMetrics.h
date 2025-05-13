#pragma once

#include <Arduino.h>

#define ARDUINOJSON_DEFAULT_NESTING_LIMIT 20 
#include <ArduinoJson.h>

class PcMetrics {
   private:
   public:
    bool is_available = false;
    bool is_updated = false;

    uint8_t cpu_temperature = 0;
    uint8_t gpu_temperature = 0;

    uint8_t cpu_load = 0;
    uint8_t gpu_load = 0; // not available
    uint8_t mem_load = 0;
    float cpu_thread_load[20] = {};

    uint16_t cpu_power = 0;
    uint16_t gpu_power = 0; // not available
 
    uint16_t cpu_fan = 0;
    uint16_t gpu_fan = 0;
    uint16_t front_fan = 0;
    uint16_t back_fan = 0;

    uint8_t gpu_3d = 0;
    uint8_t gpu_compute = 0;  // compute 1
    uint8_t gpu_decode = 0;   // first
    uint8_t gpu_mem = 0;

    float eth_up = 0;
    float eth_dn = 0;
};