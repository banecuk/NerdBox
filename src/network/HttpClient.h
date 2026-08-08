#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

class HttpClient {
 public:
    HttpClient();
    ~HttpClient();

    // connectTimeoutMs/responseTimeoutMs default to LAN-appropriate values
    // (NerdWinSense is on the same network); pass longer values for
    // internet-facing endpoints (e.g. AirVisual) where round-trips are
    // legitimately slower.
    //
    // maxRetries is actually a total-attempts cap, not a retry count: the
    // default of 2 means up to 2 GETs total (1 initial + 1 retry), not 2
    // retries after the first attempt. Kept as-is since it's the established
    // name across every call site's config field (e.g. AppSettings' various
    // `*MaxRetries` members); read it as "max attempts".
    bool download(const char* url, String& outResponse, uint8_t maxRetries = 2,
                  uint32_t retryDelayMs = 100, uint16_t connectTimeoutMs = kDefaultConnectTimeoutMs,
                  uint16_t responseTimeoutMs = kDefaultResponseTimeoutMs);

    // Deserializes the response body straight from the HTTP stream into doc,
    // instead of buffering the whole payload into a String first — skips a
    // full-payload heap copy on every call. Retries on HTTP-level failure
    // (non-200) same as download(); a parse failure after a 200 response is
    // not retried, matching download()+deserializeJson()'s prior behavior.
    // maxRetries is a total-attempts cap — see download()'s comment.
    bool downloadAndParse(const char* url, JsonDocument& doc, const JsonDocument& filter,
                          uint8_t maxRetries = 2, uint32_t retryDelayMs = 100,
                          uint16_t connectTimeoutMs = kDefaultConnectTimeoutMs,
                          uint16_t responseTimeoutMs = kDefaultResponseTimeoutMs);

    int getLastHttpCode() const { return lastHttpCode_; }
    DeserializationError getLastParseError() const { return lastParseError_; }
    // First kMaxErrorBodyChars of the response body on a non-200 (typically an
    // API's error JSON) — populated for diagnostics, empty after a 200.
    const String& getLastErrorBody() const { return lastErrorBody_; }

 private:
    // LAN-appropriate defaults — NerdWinSense is on the same network, so a
    // slow response almost certainly means the request is stuck, not just
    // crossing the internet. Keeps a failed fetch cycle from stalling the
    // background task for the HTTPClient library's default 5 s timeouts.
    static constexpr uint16_t kDefaultConnectTimeoutMs = 1000;
    static constexpr uint16_t kDefaultResponseTimeoutMs = 2000;

    HTTPClient http_;
    int lastHttpCode_ = 0;
    DeserializationError lastParseError_ = DeserializationError::Ok;
    String lastErrorBody_;
    static constexpr size_t kMaxErrorBodyChars = 200;
};