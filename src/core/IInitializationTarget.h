#pragma once

#include "ui/screens/ScreenTypes.h"
#include "utils/LoggerInterface.h"

/**
 * Narrow interface that exposes only the operations InitializationStateMachine
 * actually needs.  ApplicationComponents implements it; the state machine never
 * sees the full component aggregate.
 *
 * Grouping rationale
 * ------------------
 * - Logging      : forwarded straight to the existing LoggerInterface.
 * - Display      : initialize() + postInitialization() — two lifecycle hooks.
 * - UI           : initialize() + requestScreen() — two lifecycle hooks.
 * - Tasks        : createTasks() — single boot-time call.
 * - Network      : connect() + isConnected() — WiFi lifecycle.
 * - Time         : syncTime() — NTP call.
 * - Web server   : begin() — conditional start after network is up.
 * - System flags : three one-way setters that record boot-phase completion.
 * - Init config  : five read-only values that govern retry/backoff behaviour.
 */
class IInitializationTarget {
 public:
    virtual ~IInitializationTarget() = default;

    // --- Logging ------------------------------------------------------------
    virtual LoggerInterface& logger() = 0;

    // --- Display ------------------------------------------------------------
    virtual void initializeDisplay() = 0;
    virtual void postInitializeDisplay() = 0;

    // --- UI -----------------------------------------------------------------
    virtual void initializeUi() = 0;
    virtual void requestScreen(ScreenName screen) = 0;

    // --- Tasks --------------------------------------------------------------
    virtual bool createTasks() = 0;

    // --- Network ------------------------------------------------------------
    virtual bool connectNetwork() = 0;
    virtual bool isNetworkConnected() const = 0;

    // --- Time ---------------------------------------------------------------
    virtual bool syncTime() = 0;

    // --- Web server ---------------------------------------------------------
    virtual void beginWebServer() = 0;

    // --- System-state flags (one-way; set once during boot) -----------------
    virtual void setScreenInitialized() = 0;
    virtual void setTimeSynced() = 0;
    virtual void setSystemInitialized() = 0;

    // --- Initialisation config ----------------------------------------------
    virtual uint8_t initTimeSyncRetries() const = 0;
    virtual uint32_t initTimeSyncBaseDelayMs() const = 0;
    virtual uint16_t initBackoffJitterMs() const = 0;
    virtual bool watchdogEnabledOnBoot() const = 0;
    virtual unsigned long watchdogTimeoutMs() const = 0;
};
