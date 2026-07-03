#include "HttpClient.h"

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
        http_.begin(url);
        lastHttpCode_ = http_.GET();

        if (lastHttpCode_ == HTTP_CODE_OK) {
            outResponse = http_.getString();
            success = true;
        } else {
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
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
        http_.begin(url);
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

        retryCount++;
        if (retryCount < maxRetries) {
            vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
        }
        http_.end();
    }

    return success;
}