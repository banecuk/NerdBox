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
#include "ui/screens/ScreenFactory.h"

// *** LCD CORRUPTION FROM MEMORY LAYOUT — ROOT CAUSE FOUND AND FIXED 2026-11-07 ***
//
// Symptom (seen four times): after a change that only grew the size of
// ApplicationComponents or something embedded in it, the LCD showed wrong
// colours, the previous screen's image under the new one, and scattered
// artifacts. Clean debug/release builds and passing `pio test -e native`
// never caught it.
//
// Cause: LovyanGFX's ESP32-S3 parallel bus driver (Bus_Parallel8) DMAs
// pixel/command data out of a buffer embedded *inside* the LGFX object
// (`uint32_t _cache[2][64]`, see .pio/libdeps/.../esp32s3/Bus_Parallel8.hpp).
// LGFX used to be an inline member of PlatformBundle, which is inline in
// this class, which main.cpp heap-allocates as one large block. So the DMA
// buffer's memory region and alignment depended on sizeof(ApplicationComponents):
// adding a few bytes anywhere in here (even to an unrelated service) moved it,
// and the panel received garbage. The 2026-11-07 trigger was three
// JsonHttpService subclasses each gaining a 64-byte `host_` buffer.
//
// Fix: PlatformBundle now allocates the LGFX object on its own, in internal
// DMA-capable RAM (`heap_caps_malloc(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)`,
// see PlatformBundle::makeDisplay()). Confirmed on hardware: the same change
// set that corrupted the LCD renders cleanly once LGFX moved out.
//
// Rules that keep it fixed:
//   1. Never embed LGFX (or any other object with DMA buffers inside it —
//      LovyanGFX buses/panels, an LGFX_Sprite's backing store, an esp_lcd
//      handle with inline buffers) inline in ApplicationComponents, a bundle,
//      or anything else that's heap-allocated without MALLOC_CAP_DMA. Give it
//      its own heap_caps_malloc(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)
//      allocation like PlatformBundle::display.
//   2. Don't "fix" a recurrence by moving whichever member last grew behind a
//      unique_ptr. That's what the three earlier incidents did; it only
//      shifted the layout back to a lucky spot. If the symptom comes back,
//      look for a DMA buffer that's living inside a size-dependent allocation.
//   3. Changes touching memory layout still need a look at the real display
//      before being called done — a clean build doesn't prove it.
//
// Earlier incidents, all from this cause and all "fixed" by moving members
// to the heap: Colors' lookup tables (2026-08-15); DataBundle::cpuClockData +
// JobBundle::cpuClockStreamJob (2026-08-21); JobBundle::pcMetricsStreamJob's
// path_[96] (2026-08-30). Those moves are harmless and can stay.
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

    bool initWatchdog(uint32_t timeoutMs) override;
    bool addCurrentTaskToWatchdog() override;

    bool createTasks() override;

    bool connectNetwork() override;
    bool isNetworkConnected() const override;

    bool syncTime() override;

    void beginWebServer() override;

    void setScreenInitialized() override;
    void setSystemInitialized() override;

    void delayMs(uint32_t ms) override;
    uint32_t randomBelow(uint32_t bound) override;
    uint32_t freeHeap() const override;

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

    // UI controller — depends on platform (displayContext/displayManager),
    // services (systemMetrics), data (systemState, weatherData), and config.
    UiController uiController;

    // Every dependency a screen might need, built once here (where all of it
    // already lives) rather than re-gathered by UiController on every screen
    // transition — see docs-local/12-code-architecture.md, C4. Declared
    // after uiController (construction order = declaration order) so
    // `controller` below can reference the already-constructed uiController;
    // bound onto uiController itself in the constructor body, since
    // uiController must exist before screenCtx does, and screenCtx must
    // exist before uiController can hold a pointer to it.
    ScreenCreationContext screenCtx;

    // Managers — depend on everything above.
    TaskManager taskManager;

    // Web server — depends on uiController, services, taskManager (stack
    // high-water marks in /api/status), so it's declared after it.
    WebServer webServer;
    WebServerService webServerService;

    InitializationStateMachine initStateMachine;  // depends on *this; must stay last
};
