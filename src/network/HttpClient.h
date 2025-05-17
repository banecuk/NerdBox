#pragma once

#include <HTTPClient.h>
#include <WiFiClient.h>

class HttpClient {
   public:
    HttpClient();
    ~HttpClient();

    bool download(const char* url, String& outResponse, uint8_t maxRetries = 2,
                  uint32_t retryDelayMs = 100);

    int getLastHttpCode() const { return lastHttpCode_; }

   private:
    HTTPClient http_;
    int lastHttpCode_ = 0;
};