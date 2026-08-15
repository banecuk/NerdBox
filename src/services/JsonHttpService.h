#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "network/NetworkManager.h"
#include "utils/logging/LoggerInterface.h"

// Shared skeleton for "GET one internet-facing JSON endpoint, filter it, parse
// it into a plain data struct" services — AirQualityService and WeatherService
// were byte-for-byte identical here and had already drifted apart (only one of
// them logged the error body).
//
// CRTP rather than virtuals: every hook is known at compile time and each
// service is instantiated exactly once, so a vtable would buy nothing.
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

        HttpClient& http = networkManager_.getHttpClient();
        doc_->clear();
        // These are internet-facing APIs, not LAN-local like NerdWinSense —
        // more headroom than HttpClient's default LAN timeouts, so a
        // legitimately slow response isn't treated as a failure.
        if (!http.downloadAndParse(url_, *doc_, filter_, kMaxAttempts, kRetryDelayMs,
                                   kConnectTimeoutMs, kResponseTimeoutMs)) {
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
    // this object — in practice a string literal from Environment.h.
    JsonHttpService(NetworkManager& networkManager, LoggerInterface& logger, const char* name,
                    const char* url)
        : logger_(logger),
          networkManager_(networkManager),
          name_(name),
          url_(url),
          doc_(std::make_unique<JsonDocument>()) {}

    ~JsonHttpService() = default;

    void init() { static_cast<TDerived*>(this)->initFilter(filter_); }

    JsonDocument& doc() { return *doc_; }

    LoggerInterface& logger_;

 private:
    static constexpr uint8_t kMaxAttempts = 2;
    static constexpr uint32_t kRetryDelayMs = 100;
    static constexpr uint16_t kConnectTimeoutMs = 3000;
    static constexpr uint16_t kResponseTimeoutMs = 6000;

    NetworkManager& networkManager_;
    const char* name_;
    const char* url_;

    // Reused across fetches to avoid heap fragmentation.
    std::unique_ptr<JsonDocument> doc_;
    JsonDocument filter_;  // Filter built once (small size)
};
