#pragma once

#include <Arduino.h>

// Written by NetworkStatusService (background/probe tasks),
// read by NetworkWidget (screen task).
// All scalar fields are Xtensa word-sized — naturally atomic, no mutex needed.
struct NetworkStatus {
    // WiFi / LAN — updated every background loop tick
    bool   wifi_connected = false;
    int8_t rssi           = 0;  // dBm; valid only when wifi_connected

    // Internet reachability — updated after each probe round
    enum class Internet : uint8_t {
        UNKNOWN,   // no probe completed yet
        OK,        // all recent probes passed
        DEGRADED,  // mixed results
        DOWN       // all recent probes failed
    } internet = Internet::UNKNOWN;

    // Set true while the one-shot probe task is in-flight; guards against
    // spawning overlapping tasks.
    volatile bool probe_running = false;

    // millis() of the last completed probe — used by TaskManager to pace retriggers
    unsigned long last_probe = 0;
};
