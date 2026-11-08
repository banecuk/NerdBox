#pragma once

#include <cstdint>

#include "core/ScreenTypes.h"
#include "utils/logging/LoggerInterface.h"

/**
 * Narrow interface that exposes only the operations InitializationStateMachine
 * actually needs.  ApplicationComponents implements it; the state machine never
 * sees the full component aggregate.
 *
 * Every hardware/platform call the boot sequence makes goes through here
 * (watchdog, delay, random jitter, heap stats), so the state machine itself
 * has no Arduino/ESP-IDF dependency and is host-tested under `[env:native]`
 * (test/InitializationStateMachineTest.cpp) with a fake target. Tuning values
 * (retry counts, backoff, watchdog enable/timeout) are NOT here — the state
 * machine reads them straight from `const AppSettings&`.
 *
 * Grouping rationale
 * ------------------
 * - Logging      : forwarded straight to the existing LoggerInterface.
 * - Display      : initialize() + postInitialization() — two lifecycle hooks.
 * - UI           : initialize() + requestScreen() — two lifecycle hooks.
 * - Watchdog     : arm the task WDT, then join the calling (setup) task.
 *                  TaskManager owns the watchdog and records what really got
 *                  armed (B6).
 * - Tasks        : createTasks() — single boot-time call.
 * - Network      : connect() + isConnected() — WiFi lifecycle.
 * - Time         : syncTime() — NTP call.
 * - Web server   : begin() — conditional start after network is up.
 * - System flags : two one-way setters that record boot-phase completion.
 * - Platform     : delayMs(), randomBelow(), freeHeap() — the bits of
 *                  Arduino/ESP the boot sequence needs, injected so a host
 *                  test can record/fake them.
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
    virtual bool initializeUi() = 0;
    virtual void requestScreen(ScreenName screen) = 0;

    // --- Watchdog -----------------------------------------------------------
    // Arms the task watchdog with the given timeout. Called before any task
    // exists. Returns whether the watchdog is actually armed; on false nothing
    // will be added to it or reset later (B6).
    virtual bool initWatchdog(uint32_t timeoutMs) = 0;
    // Adds the calling task (the Arduino setup/loop task) to the watchdog.
    // No-op if initWatchdog() didn't succeed. Returns whether it joined.
    virtual bool addCurrentTaskToWatchdog() = 0;

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
    virtual void setSystemInitialized() = 0;

    // --- Platform -----------------------------------------------------------
    // Blocks the calling task for `ms` milliseconds.
    virtual void delayMs(uint32_t ms) = 0;
    // Uniform random value in [0, bound); 0 when bound == 0.
    virtual uint32_t randomBelow(uint32_t bound) = 0;
    virtual uint32_t freeHeap() const = 0;
};
