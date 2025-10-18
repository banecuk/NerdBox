#include "HttpClient.h"

HttpClient::HttpClient() {}

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