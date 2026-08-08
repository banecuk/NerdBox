#include "HttpClient.h"

namespace {
// Only transient failures are worth retrying: negative codes are
// HTTPClient-level errors (connection refused, timeout, DNS failure) and 5xx
// are server-side. 4xx means the request itself is malformed/rejected and
// retrying it will fail identically every time.
bool isRetryable(int httpCode) {
    return httpCode < 0 || httpCode >= 500;
}
}  // namespace

HttpClient::HttpClient() {
    http_.setReuse(true);  // keep-alive across the same host; begin() reconnects when the host changes
}

HttpClient::~HttpClient() {
    http_.end();
}

bool HttpClient::download(const char* url, String& outResponse, uint8_t maxRetries,
                          uint32_t retryDelayMs, uint16_t connectTimeoutMs,
                          uint16_t responseTimeoutMs) {
    bool success = false;
    uint8_t retryCount = 0;
    lastHttpCode_ = 0;

    while (retryCount < maxRetries && !success) {
        if (!http_.begin(url)) {
            // begin() itself doesn't report a specific failure code — it fails
            // on a malformed/unparseable URL just as often as an actual
            // connection problem. HTTPC_ERROR_CONNECTION_REFUSED is the closest
            // existing HTTPClient error code, not a literal diagnosis.
            lastHttpCode_ = HTTPC_ERROR_CONNECTION_REFUSED;
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
            continue;
        }

        http_.setConnectTimeout(connectTimeoutMs);
        http_.setTimeout(responseTimeoutMs);
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
            http_.end();  // Unread body on a kept-alive socket would desync the next request
            break;  // Non-retryable HTTP error (e.g. 4xx) — retrying won't help
        }
        http_.end();
    }

    return success;
}

bool HttpClient::downloadAndParse(const char* url, JsonDocument& doc, const JsonDocument& filter,
                                  uint8_t maxRetries, uint32_t retryDelayMs,
                                  uint16_t connectTimeoutMs, uint16_t responseTimeoutMs) {
    bool success = false;
    uint8_t retryCount = 0;
    lastHttpCode_ = 0;
    lastParseError_ = DeserializationError::Ok;

    while (retryCount < maxRetries && !success) {
        if (!http_.begin(url)) {
            // begin() itself doesn't report a specific failure code — it fails
            // on a malformed/unparseable URL just as often as an actual
            // connection problem. HTTPC_ERROR_CONNECTION_REFUSED is the closest
            // existing HTTPClient error code, not a literal diagnosis.
            lastHttpCode_ = HTTPC_ERROR_CONNECTION_REFUSED;
            retryCount++;
            if (retryCount < maxRetries) {
                vTaskDelay(pdMS_TO_TICKS(retryDelayMs));
            }
            continue;
        }

        http_.setConnectTimeout(connectTimeoutMs);
        http_.setTimeout(responseTimeoutMs);
        // Forces HTTP/1.0 semantics (no "Transfer-Encoding: chunked", no
        // keep-alive) so getStream() always yields a plain Content-Length
        // body that deserializeJson() can parse directly — the standard
        // ArduinoJson streaming-parse recipe.
        http_.useHTTP10(true);
        lastHttpCode_ = http_.GET();

        if (lastHttpCode_ == HTTP_CODE_OK) {
            lastErrorBody_.clear();
            lastParseError_ =
                deserializeJson(doc, http_.getStream(), DeserializationOption::Filter(filter));
            success = !lastParseError_;
            http_.end();
            break;  // Parse is attempted exactly once per successful fetch —
                     // matches the old download()+deserializeJson() behavior
                     // of retrying only on HTTP-level failure, not on a
                     // malformed 200 response.
        }

        // Non-200: grab the body for diagnostics before end() closes the
        // socket (unread body on keep-alive would desync the next request).
        lastErrorBody_ = http_.getString();
        if (lastErrorBody_.length() > kMaxErrorBodyChars) {
            lastErrorBody_.remove(kMaxErrorBodyChars);
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