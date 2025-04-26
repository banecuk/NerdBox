// HttpDownloader.h
#ifndef HTTP_DOWNLOADER_H
#define HTTP_DOWNLOADER_H

#include <HTTPClient.h>
#include <WiFiClient.h>

class HttpDownloader {
   public:
    HttpDownloader();
    ~HttpDownloader();

    bool download(const char* url, String& outResponse, uint8_t maxRetries = 2,
                  uint32_t retryDelayMs = 100);

    int getLastHttpCode() const { return lastHttpCode_; }

   private:
    HTTPClient http_;
    int lastHttpCode_ = 0;
};

#endif  // HTTP_DOWNLOADER_H