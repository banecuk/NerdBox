#include "HttpClient.h"

namespace {
// LAN-appropriate timeouts — NerdWinSense is on the same network, so a slow
// response almost certainly means the request is stuck, not just crossing
// the internet. Keeps a failed fetch cycle from stalling the background
// task for the HTTPClient library's default 5 s timeouts.
constexpr uint16_t kConnectTimeoutMs = 1000;
constexpr uint16_t kResponseTimeoutMs = 2000;

// Only transient failures are worth retrying: negative codes are
// HTTPClient-level errors (connection refused, timeout, DNS failure) and 5xx
// are server-side. 4xx means the request itself is malformed/rejected and
// retrying it will fail identically every time.
bool isRetryable(int httpCode) {
    return httpCode < 0 || httpCode >= 500;
}
}  // namespace

HttpClient::HttpClient() {
    // Keeps the TCP connection open across requests to the same host (sends
    // "Connection: keep-alive" and skips the handshake on the next begin()
    // if the socket is still connected) instead of reconnecting on every
    // poll. download() still calls end() after every request as before —
    // with reuse enabled, end() only closes the socket when the server
    // actually asked for it (HTTP/1.0 or "Connection: close") or the
    // request failed, so no other code needs to change.
    http_.setReuse(true);
}

HttpClient::~HttpClient() {
    http_.end();
}

bool HttpClient::download(const char* url, String& outResponse, uint8_t maxRetries,
                          uint32_t retryDelayMs) {
    bool success = false;
    uint8_t retryCount = 0;
    lastHttpCode_ = 0;

    while (retryCount < maxRetries && !success) {
        if (!http_.begin(url)) {
            lastHttpCode_ = HTTPC_ERROR_CONNECTION_REFUSED;
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
            continue;
        }

        http_.setConnectTimeout(kConnectTimeoutMs);
        http_.setTimeout(kResponseTimeoutMs);
        lastHttpCode_ = http_.GET();

        if (lastHttpCode_ == HTTP_CODE_OK) {
            outResponse = http_.getString();
            success = true;
        } else if (isRetryable(lastHttpCode_)) {
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
        } else {
            break;  // Non-retryable HTTP error (e.g. 4xx) — retrying won't help
        }
        http_.end();
    }

    return success;
}

bool HttpClient::downloadAndParse(const char* url, JsonDocument& doc, const JsonDocument& filter,
                                  uint8_t maxRetries, uint32_t retryDelayMs) {
    bool success = false;
    uint8_t retryCount = 0;
    lastHttpCode_ = 0;
    lastParseError_ = DeserializationError::Ok;

    while (retryCount < maxRetries && !success) {
        if (!http_.begin(url)) {
            lastHttpCode_ = HTTPC_ERROR_CONNECTION_REFUSED;
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
            continue;
        }

        http_.setConnectTimeout(kConnectTimeoutMs);
        http_.setTimeout(kResponseTimeoutMs);
        lastHttpCode_ = http_.GET();

        if (lastHttpCode_ == HTTP_CODE_OK) {
            lastParseError_ =
                deserializeJson(doc, http_.getStream(), DeserializationOption::Filter(filter));
            success = !lastParseError_;
            http_.end();
            break;  // Parse is attempted exactly once per successful fetch —
                     // matches the old download()+deserializeJson() behavior
                     // of retrying only on HTTP-level failure, not on a
                     // malformed 200 response.
        }

        http_.end();
        if (!isRetryable(lastHttpCode_)) {
            break;  // Non-retryable HTTP error (e.g. 4xx) — retrying won't help
        }

        retryCount++;
        if (retryCount < maxRetries) {
            vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
        }
    }

    return success;
}