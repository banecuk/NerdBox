#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <atomic>
#include <cstring>

#include "config/Limits.h"
#include "network/LinkInfo.h"
#include "utils/PublishedFlag.h"
#include "utils/ScopedLock.h"

// Written by NetworkStatusService (background/probe tasks),
// read by NetworkWidget (screen task).
// All scalar fields are Xtensa word-sized — naturally atomic, no mutex needed.
struct NetworkStatus {
    NetworkStatus() { memset(&link_, 0, sizeof(link_)); }
    ~NetworkStatus() { vSemaphoreDelete(linkMutex_); }
    NetworkStatus(const NetworkStatus&) = delete;
    NetworkStatus& operator=(const NetworkStatus&) = delete;

    // This device's own link details (A8), refreshed by NetworkStatusService
    // on the background task and read by WifiLinkWidget/IpAddressWidget
    // (screen task) and /api/status, /metrics (web task). LinkInfo is ~200 B
    // of strings, so unlike the scalars below it's guarded by a mutex: copy
    // it out with copyLink(), never hold the lock across drawing or I/O.
    void copyLink(LinkInfo& out) const {
        ScopedLock lock(linkMutex_);
        out = link_;
    }
    void storeLink(const LinkInfo& in, unsigned long nowMs) {
        {
            ScopedLock lock(linkMutex_);
            link_ = in;
        }
        linkFreshness.publish(nowMs);
    }
    // available() once the first snapshot has been stored.
    PublishedFlag linkFreshness;

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

    // Set true while a probe is in flight on NetworkStatusService's persistent
    // probe task; guards against waking it again (and rewriting its context)
    // mid-probe. std::atomic (not volatile) so the probe task's writes to
    // last_probe/results/endpoint_ok/internet are guaranteed visible to the
    // background task once it observes probe_running go false — volatile
    // gives no such ordering on Xtensa.
    std::atomic<bool> probe_running{false};

    // millis() of the last completed probe — used by TaskManager to pace retriggers
    unsigned long last_probe = 0;

 private:
    LinkInfo link_;
    SemaphoreHandle_t linkMutex_ = xSemaphoreCreateMutex();
};
