#pragma once

#include <WebServer.h>

#include "app/bundles/DataBundle.h"
#include "app/bundles/JobBundle.h"
#include "app/bundles/PlatformBundle.h"
#include "app/bundles/ServiceBundle.h"
#include "app/InitializationStateMachine.h"
#include "app/TaskManager.h"
#include "config/AppSettings.h"
#include "core/IInitializationTarget.h"
#include "services/web/WebServerService.h"
#include "ui/core/UiController.h"

// *** SIZE-SENSITIVE ON REAL HARDWARE — READ BEFORE ADDING/GROWING A MEMBER ***
//
// ApplicationComponents (heap-allocated as one block via
// std::make_unique<ApplicationComponents>() in main.cpp, alongside the LGFX
// display) has, three separate times, corrupted the whole LCD on real
// hardware (scattered pixels, no clear on screen transitions) after a change
// that grew the size of something embedded *inline* inside it or one of its
// bundles (DataBundle/PlatformBundle/ServiceBundle/JobBundle) — even though
// the change built cleanly and every `[env:native]` host test passed. Root
// cause isn't fully pinned down (plausibly a tight internal-RAM margin
// against the display driver's own DMA-capable allocations), but the size
// correlation has been confirmed by hardware bisection every time: adding a
// couple hundred bytes of new per-instance state reproduces it, and moving
// that same state behind a heap pointer fixes it.
//
// So: a new field on any struct embedded here, a bigger fixed array, an
// added scratch buffer (SseConnection, JsonDocument, etc.) — anything that
// grows `sizeof()` of a member of ApplicationComponents or one of its
// bundles — needs either:
//   1. std::unique_ptr<T> (heap-allocated, e.g. via std::make_unique) instead
//      of an inline T member, or
//   2. a `static` member instead of per-instance state, if the type is only
//      ever constructed once (no state actually needs to live per-instance).
// A clean debug/release build and passing `pio test -e native` are NOT
// sufficient evidence the change is safe — they were passing all three times
// this broke. Flash the device and visually check the display before
// reporting such a change as complete; if hardware isn't available, say the
// change is unverified rather than "done".
//
// When a feature adds new members across *multiple* bundles, convert every
// new embedded member to a heap pointer up front — fixing just one and
// reflashing to check is not a valid way to bisect which one mattered (see
// the CPU-clock-screen incident: CpuClockData alone wasn't enough; the
// paired CpuClockStreamJob also had to move to the heap).
//
// History: Colors' two lookup tables (2026-08-15, docs-local/07-performance
// P1-6); DataBundle::cpuClockData + JobBundle::cpuClockStreamJob
// (2026-08-21, CPU-clock screen); JobBundle::pcMetricsStreamJob growing a
// path_[96] member when it moved onto the shared SseStreamJob base
// (2026-08-30, docs-local/11-code-quality.md Q1) — see
// [[applicationcomponents-size-sensitive]] in memory for the full incident
// log.
class ApplicationComponents : public IInitializationTarget {
 public:
    ApplicationComponents();
    ~ApplicationComponents() = default;

    // Delete copy/move operations
    ApplicationComponents(const ApplicationComponents&) = delete;
    ApplicationComponents& operator=(const ApplicationComponents&) = delete;
    ApplicationComponents(ApplicationComponents&&) = delete;
    ApplicationComponents& operator=(ApplicationComponents&&) = delete;

    // -----------------------------------------------------------------------
    // IInitializationTarget implementation — see ApplicationComponents.cpp
    // -----------------------------------------------------------------------
    LoggerInterface& logger() override;

    void initializeDisplay() override;
    void postInitializeDisplay() override;

    bool initializeUi() override;
    void requestScreen(ScreenName screen) override;

    bool createTasks() override;

    bool connectNetwork() override;
    bool isNetworkConnected() const override;

    bool syncTime() override;

    void beginWebServer() override;

    void setScreenInitialized() override;
    void setTimeSynced() override;
    void setSystemInitialized() override;

    uint8_t initTimeSyncRetries() const override;
    uint32_t initTimeSyncBaseDelayMs() const override;
    uint16_t initBackoffJitterMs() const override;
    bool watchdogEnabledOnBoot() const override;
    unsigned long watchdogTimeoutMs() const override;

    // -----------------------------------------------------------------------
    // Public data — accessed by Application, TaskManager wiring, etc.
    //
    // Declaration order matters: members construct in declaration order
    // regardless of the constructor's initializer-list order. Each bundle
    // resolves its own internal ordering constraints (see the comment in
    // each bundle header); across bundles, later ones depend on earlier ones
    // — keep that order when adding a new bundle or trio member.
    // -----------------------------------------------------------------------

    AppSettings config;

    DataBundle data;
    PlatformBundle platform;
    ServiceBundle services;
    JobBundle jobs;

    // UI controller — depends on platform (displayContext/displayManager/
    // networkManager), services (systemMetrics), data (pcMetrics,
    // systemState, airQualityData, netStatus, weatherData), and config.
    UiController uiController;

    // Managers — depend on everything above.
    TaskManager taskManager;

    // Web server — depends on uiController, services, taskManager (stack
    // high-water marks in /api/status), so it's declared after it.
    WebServer webServer;
    WebServerService webServerService;

    InitializationStateMachine initStateMachine;  // depends on *this; must stay last
};
