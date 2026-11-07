#pragma once

#include <LovyanGFX.hpp>
#include <esp_heap_caps.h>

#include <memory>
#include <new>

#include "config/AppSettings.h"
#include "network/HttpClient.h"
#include "network/NetworkManager.h"
#include "services/NtpService.h"
#include "ui/core/Colors.h"
#include "ui/core/DisplayContext.h"
#include "ui/core/DisplayManager.h"
#include "utils/logging/Logger.h"

// Hardware + platform plumbing: display, colors, HTTP transport, the logger,
// network connectivity, and NTP. Declaration order matters — members
// construct in this order regardless of the constructor's initializer-list
// order, and several depend on earlier ones (logger_ on isTimeSynced,
// displayContext on display/colors/logger_, displayManager on
// display/logger_/config, networkManager on logger_/httpClient/config).
// Keep new members near what they depend on, before whatever depends on them.
//
// The display is NOT inline in this bundle on purpose — see `display` below
// and the LCD-corruption note at the top of app/ApplicationComponents.h.
struct PlatformBundle {
    // DO NOT turn this back into an inline `LGFX display;` member.
    //
    // The LGFX object gets its own allocation in internal, DMA-capable RAM.
    // LovyanGFX's ESP32-S3 Bus_Parallel8 DMAs pixel/command data out of a
    // buffer embedded in the object itself (`_cache[2][64]`). Embedded
    // inline here, that buffer lived wherever the whole ApplicationComponents
    // heap block landed, so any size change anywhere in ApplicationComponents
    // could move it and corrupt the LCD (wrong colours, old image showing
    // through). Confirmed on hardware 2026-11-07 — see the note at the top
    // of app/ApplicationComponents.h.
    struct LgfxDeleter {
        void operator()(LGFX* p) const {
            p->~LGFX();
            heap_caps_free(p);
        }
    };
    static std::unique_ptr<LGFX, LgfxDeleter> makeDisplay() {
        void* mem = heap_caps_malloc(sizeof(LGFX), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        if (mem == nullptr) {
            abort();  // boot-fatal: there is no display without it
        }
        return std::unique_ptr<LGFX, LgfxDeleter>(new (mem) LGFX());
    }
    std::unique_ptr<LGFX, LgfxDeleter> displayStorage_ = makeDisplay();
    LGFX& display = *displayStorage_;
    Colors colors;
    HttpClient httpClient;

    // named logger_ to match IInitializationTarget::logger()'s use of this
    // member elsewhere in ApplicationComponents
    Logger logger_;

    DisplayContext displayContext;
    DisplayManager displayManager;

    NetworkManager networkManager;
    NtpService ntpService;

    // isTimeSynced comes from DataBundle::systemState.core, constructed
    // before this bundle in ApplicationComponents' declaration order.
    PlatformBundle(const bool& isTimeSynced, const AppSettings& config)
        : logger_(isTimeSynced),
          displayContext(display, colors, logger_, logger_),
          displayManager(display, logger_, config),
          networkManager(logger_, httpClient, config) {}
};
