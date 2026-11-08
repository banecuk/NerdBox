#pragma once

#include <cstddef>

#include "services/network/NetworkStatus.h"

// One rolling probe slot: whether it has ever been probed, and whether its
// most recent probe passed.
struct ProbeResult {
    bool probed = false;
    bool ok = false;
};

// Pure verdict over the rolling probe slots, extracted from
// NetworkStatusService::recordResult() so it's host-tested
// (test/InternetVerdictTest.cpp).
//
// Only slots that have been probed at least once count — a never-probed slot
// is neither a pass nor a fail, which keeps the status accurate during the
// warm-up window right after boot.
//
// UNKNOWN  — no slot probed yet
// DOWN     — every probed slot failed
// OK       — at most 1 probed slot failed. These are third-party endpoints;
//            one of them going down (or rate-limiting us) says nothing about
//            this device's connectivity, so a lone failure is not flagged —
//            its dot still turns red in NetworkWidget, the globe stays white.
// WARNING  — exactly 2 probed slots failed
// DEGRADED — 3 or more probed slots failed, but not all of them
NetworkStatus::Internet classifyInternet(const ProbeResult* results, size_t count);
