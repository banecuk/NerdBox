#pragma once

#include <Arduino.h>

#include <atomic>

#include "config/Limits.h"

// Written by NetworkStatusService (background/probe tasks),
// read by NetworkWidget (screen task).
// All scalar fields are Xtensa word-sized — naturally atomic, no mutex needed.
struct NetworkStatus {
    // WiFi / LAN — updated every background loop tick
    bool wifi_connected = false;
    int8_t rssi = 0;  // dBm; valid only when wifi_connected

    // Internet reachability — updated after each probe round
    enum class Internet : uint8_t {
        UNKNOWN,   // no probe completed yet
        OK,        // all recent probes passed
        WARNING,   // exactly one service failed
        DEGRADED,  // two or more services failed (but not all)
        DOWN       // all recent probes failed
    } internet = Internet::UNKNOWN;

    // Per-endpoint last-known pass/fail (true=pass, false=fail/unknown).
    // Index matches NetworkStatusService::kProbeUrls[]. Sized off the same
    // shared constant NetworkStatusService::kNumEndpoints derives from
    // (AppConfig::Limits::kNetworkProbeEndpoints) so the two can't drift.
    // Updated atomically (bool is word-sized on Xtensa) — no mutex needed.
    bool endpoint_ok[AppConfig::Limits::kNetworkProbeEndpoints] = {};

    // Set true while the one-shot probe task is in-flight; guards against
    // spawning overlapping tasks. std::atomic (not volatile) so the probe
    // task's writes to last_probe/results/endpoint_ok/internet are guaranteed
    // visible to the background task on the other core once it observes
    // probe_running go false — volatile gives no such ordering on Xtensa.
    std::atomic<bool> probe_running{false};

    // millis() of the last completed probe — used by TaskManager to pace retriggers
    unsigned long last_probe = 0;
};
