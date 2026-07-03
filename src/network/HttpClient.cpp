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