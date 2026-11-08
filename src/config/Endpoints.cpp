#include "Endpoints.h"

// The one and only Environment.h include outside config/Environment.h itself
// (A9) — see the CI layering check in .github/workflows/ci.yml.
#include "config/Environment.h"

Endpoints Endpoints::fromEnvironment() {
    Endpoints e;
    e.wifiSsid = WIFI_SSID;
    e.wifiPassword = WIFI_PASSWORD;
    e.pcMetricsApi = LIBRE_HM_API;
    e.airVisualApi = AIR_VISUAL_API;
    e.roomClimateApi = ROOM_CLIMATE_API;
    e.weatherApi = WEATHER_API;
    e.ethDownloadMbps = ETH_DOWNLOAD_MBPS;
    e.ethUploadMbps = ETH_UPLOAD_MBPS;
    return e;
}
