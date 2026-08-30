#pragma once

#include <LovyanGFX.hpp>

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
// SIZE-SENSITIVE: this bundle is embedded inline in ApplicationComponents —
// see the warning at the top of app/ApplicationComponents.h before adding or
// growing a member. Colors is the one member that already hit this (its
// lookup tables are `static`, not per-instance, for exactly this reason).
struct PlatformBundle {
    LGFX display;
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
