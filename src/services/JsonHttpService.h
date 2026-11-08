#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "config/AppSettings.h"
#include "network/BoundedDns.h"
#include "network/NetworkManager.h"
#include "network/UrlUtils.h"
#include "utils/logging/LoggerInterface.h"

// Shared skeleton for "GET one internet-facing JSON endpoint, filter it, parse
// it into a plain data struct" services — AirQualityService and WeatherService
// were byte-for-byte identical here and had already drifted apart (only one of
// them logged the error body).
//
// CRTP rather than virtuals: every hook is known at compile time and each
// service is instantiated exactly once, so a vtable would buy nothing.
//
// Only PcMetricsService, AirQualityService, and WeatherService derive from
// this — PcMetricsStreamService (the default PC-metrics data path) is SSE,
// not a bounded request/response GET, and can't fit this pattern. Don't
// assume every service shares this base.
// Derived must provide:
//     void initFilter(JsonDocument& filter);   // called once, from init()
//     bool parseData(TData& outData);          // reads doc(), fills outData
//
// Derived's constructor must call init() once its own members are ready —
// initFilter() is a derived method and can't be called from this base's
// constructor.
template <typename TData, typename TDerived>
class JsonHttpService {
 public:
    JsonHttpService(const JsonHttpService&) = delete;
    JsonHttpService& operator=(const JsonHttpService&) = delete;

    // Fetches fresh data and writes it into outData. Returns true on success.
    bool fetchData(TData& outData) {
        if (!networkManager_.isConnected()) {
            logger_.warningf("%s: network not connected", name_);
            return false;
        }

        // Bounded DNS pre-check: HTTPClient's own lookup can block ~30 s when
        // the router's DNS forwarder stops answering, past the 20 s task
        // watchdog (B38). A success leaves the answer in lwIP's cache, so the
        // GET below resolves instantly. LAN IP literals skip this.
        if (!BoundedDns::resolve(host_, config_.httpFetchDnsTimeoutMs)) {
            logger_.warningf("%s: DNS lookup for %s failed or timed out", name_, host_);
            return false;
        }

        HttpClient& http = networkManager_.getHttpClient();
        doc_->clear();
        // These are internet-facing APIs, not LAN-local like NerdWinSense —
        // more headroom than HttpClient's default LAN timeouts, so a
        // legitimately slow response isn't treated as a failure.
        if (!http.downloadAndParse(url_, *doc_, filter_, config_.httpFetchMaxAttempts,
                                   config_.httpFetchRetryDelayMs, config_.httpFetchConnectTimeoutMs,
                                   config_.httpFetchResponseTimeoutMs)) {
            if (http.getLastHttpCode() == HTTP_CODE_OK) {
                logger_.warningf("%s: JSON parse error: %s", name_,
                                 http.getLastParseError().c_str());
            } else {
                logger_.errorf("%s: HTTP GET failed, code: %d: %s", name_, http.getLastHttpCode(),
                               http.getLastErrorBody().c_str());
            }
            return false;
        }

        return static_cast<TDerived*>(this)->parseData(outData);
    }

 protected:
    // name is a short log prefix ("Weather", "AirQuality"); url must outlive
    // this object — in practice AppSettings::endpoints (A9). Timeouts come
    // from config's httpFetch* knobs (AppConfig::internal::HttpFetchImpl).
    JsonHttpService(NetworkManager& networkManager, LoggerInterface& logger,
                    const AppSettings& config, const char* name, const char* url)
        : logger_(logger),
          networkManager_(networkManager),
          config_(config),
          name_(name),
          url_(url),
          doc_(std::make_unique<JsonDocument>()) {
        uint16_t unusedPort = 0;
        UrlUtils::parseHostPort(url_, host_, sizeof(host_), unusedPort);
    }

    ~JsonHttpService() = default;

    void init() { static_cast<TDerived*>(this)->initFilter(filter_); }

    JsonDocument& doc() { return *doc_; }

    LoggerInterface& logger_;

 private:
    // Worst case per fetchData(): DNS + connect + response timeouts ≈ 13 s
    // by default, well inside the 20 s task watchdog; AppConfig.h
    // static_asserts that the defaults stay there (B38).
    NetworkManager& networkManager_;
    const AppSettings& config_;
    const char* name_;
    const char* url_;
    char host_[64] = "";

    // Reused across fetches to avoid heap fragmentation.
    std::unique_ptr<JsonDocument> doc_;
    JsonDocument filter_;  // Filter built once (small size)
};
