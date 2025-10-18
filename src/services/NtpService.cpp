#include "NtpService.h"

#include <WiFi.h>

const char* NtpService::DEFAULT_NTP_SERVER1 = "europe.pool.ntp.org";
const char* NtpService::DEFAULT_NTP_SERVER2 = "time.google.com";
const char* NtpService::DEFAULT_NTP_SERVER3 = "time.cloudflare.com";
const uint32_t NtpService::DEFAULT_GMT_OFFSET_SEC = 3600;
const uint32_t NtpService::DEFAULT_DAYLIGHT_OFFSET_SEC = 3600;

NtpService::NtpService()
    : timeSynced_(false),
      ntpServer1_(DEFAULT_NTP_SERVER1),
      ntpServer2_(DEFAULT_NTP_SERVER2),
      ntpServer3_(DEFAULT_NTP_SERVER3),
      gmtOffsetSec_(DEFAULT_GMT_OFFSET_SEC),
      daylightOffsetSec_(DEFAULT_DAYLIGHT_OFFSET_SEC) {}

bool NtpService::syncTime(const char* timezone) {
    configTime(gmtOffsetSec_, daylightOffsetSec_, ntpServer1_, ntpServer2_, ntpServer3_);
    setenv("TZ", timezone, 1);
    tzset();

    // Wait for time to synchronize
    struct tm timeinfo;
    int retries = 0;
    const int maxRetries = 20;
    while (!getLocalTime(&timeinfo)) {
        if (retries++ >= maxRetries) {
            timeSynced_ = false;
            return false;
        }
        delay(200);
    }

    timeSynced_ = true;
    return true;
}

bool NtpService::isTimeSynced() const {
    return timeSynced_;
}

struct tm NtpService::getTime() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        memset(&timeinfo, 0, sizeof(timeinfo));
    }
    return timeinfo;
}

std::string NtpService::getFormattedTime() const {
    struct tm timeinfo = getTime();
    char buffer[8];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return std::string(buffer);
}