#pragma once

// Per-installation endpoints and secrets (A9). The values live in the
// gitignored Environment.h; Endpoints.cpp is the ONLY file that includes it
// (enforced by a CI grep). Everything else reads them through
// AppSettings::endpoints, so service/widget headers stay free of
// Environment.h and can build on [env:native].
//
// Default-constructed (empty strings, zero speeds) in AppSettings; the
// composition root fills it in with Endpoints::fromEnvironment(). A host test
// that default-constructs AppSettings therefore needs no Environment.h.
struct Endpoints {
    const char* wifiSsid = "";
    const char* wifiPassword = "";

    const char* pcMetricsApi = "";    // NerdWinSense /api/v1/system (SSE host:port derive from it)
    const char* airVisualApi = "";    // contains an API key: never dump it (/config skips it)
    const char* roomClimateApi = "";  // local LAN sensor
    const char* weatherApi = "";      // open-meteo daily forecast

    // Ethernet link speed in Mbps, for NetworkTrafficWidget's utilisation colours.
    float ethDownloadMbps = 0.0f;
    float ethUploadMbps = 0.0f;

    // Defined in Endpoints.cpp from Environment.h's constants.
    static Endpoints fromEnvironment();
};
