#pragma once

#include <cstddef>

#include "core/ScreenTypes.h"
#include "core/events/EventTypes.h"

// Single source of truth for "which screens exist" — replaces four
// hand-maintained tables (ScreenFactory's switch, WebServerService's route
// array, UiEventHandler's one-line methods, WebApiHandlers' name switch) with
// one. See docs-local/12-code-architecture.md, C1.
struct ScreenDescriptor {
    ScreenName screen;
    EventType event;    // EventType::NONE for screens with no event (BOOT)
    const char* route;  // nullptr for screens with no HTTP route (BOOT)
    const char* name;   // "MAIN", "CPU_CLOCK", ... — the /api/status string
    // True for screens that display PcMetrics. PcMetricsJob (polling) and
    // PcMetricsStreamJob (SSE) only fetch while one of these is active, via
    // screenNeedsPcMetrics() — one column here instead of a hand-written
    // screen list in each job, which could drift apart (A12).
    bool needsPcMetrics;
};

// clang-format off
inline constexpr ScreenDescriptor kScreens[] = {
    // screen               event                      route                name         needsPcMetrics
    {ScreenName::MAIN,      EventType::SHOW_MAIN,      "/screen/main",      "MAIN",      true },
    {ScreenName::SETTINGS,  EventType::SHOW_SETTINGS,  "/screen/settings",  "SETTINGS",  false},
    {ScreenName::GAME,      EventType::SHOW_GAME,      "/screen/game",      "GAME",      true },
    {ScreenName::DISKS,     EventType::SHOW_DISKS,     "/screen/disks",     "DISKS",     true },
    {ScreenName::CPU_CLOCK, EventType::SHOW_CPU_CLOCK, "/screen/cpu-clock", "CPU_CLOCK", false},
    {ScreenName::PROCESSES, EventType::SHOW_PROCESSES, "/screen/processes", "PROCESSES", false},
    {ScreenName::WEATHER,   EventType::SHOW_WEATHER,   "/screen/weather",   "WEATHER",   false},
    {ScreenName::CALENDAR,  EventType::SHOW_CALENDAR,  "/screen/calendar",  "CALENDAR",  false},
    {ScreenName::WIFI,      EventType::SHOW_WIFI,      "/screen/wifi",      "WIFI",      false},
    {ScreenName::BOOT,      EventType::NONE,           nullptr,             "BOOT",      false},
};
// clang-format on
inline constexpr size_t kScreenCount = sizeof(kScreens) / sizeof(kScreens[0]);

// Linear scan over kScreens; falls back to "NONE" (also covers ScreenName::NONE
// itself, which has no descriptor).
inline const char* screenName(ScreenName screen) {
    for (const auto& d : kScreens) {
        if (d.screen == screen) return d.name;
    }
    return "NONE";
}

// The registry row's needsPcMetrics; false for ScreenName::NONE (no row).
inline constexpr bool screenNeedsPcMetrics(ScreenName screen) {
    for (const auto& d : kScreens) {
        if (d.screen == screen) return d.needsPcMetrics;
    }
    return false;
}
