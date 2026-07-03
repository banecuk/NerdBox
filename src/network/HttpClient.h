#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

class HttpClient {
 public:
    HttpClient();
    ~HttpClient();

    bool download(const char* url, String& outResponse, uint8_t maxRetries = 2,
                  uint32_t retryDelayMs = 100);

    // Deserializes the response body straight from the HTTP stream into doc,
    // instead of buffering the whole payload into a String first — skips a
    // full-payload heap copy on every call. Retries on HTTP-level failure
    // (non-200) same as download(); a parse failure after a 200 response is
    // not retried, matching download()+deserializeJson()'s prior behavior.
    bool downloadAndParse(const char* url, JsonDocument& doc, const JsonDocument& filter,
                          uint8_t maxRetries = 2, uint32_t retryDelayMs = 100);

    int getLastHttpCode() const { return lastHttpCode_; }
    DeserializationError getLastParseError() const { return lastParseError_; }

 private:
    HTTPClient http_;
    int lastHttpCode_ = 0;
    DeserializationError lastParseError_ = DeserializationError::Ok;
};