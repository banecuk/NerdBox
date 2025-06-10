#pragma once

#include <time.h>

#include <string>

class NtpService {
   public:
    NtpService();
    bool syncTime(const char *timezone = "CET-1CEST,M3.5.0,M10.5.0/3");
    bool isTimeSynced() const;
    struct tm getTime() const;
    std::string getFormattedTime() const;

    static const char *DEFAULT_NTP_SERVER1;
    static const char *DEFAULT_NTP_SERVER2;
    static const char *DEFAULT_NTP_SERVER3;
    static const uint32_t DEFAULT_GMT_OFFSET_SEC;
    static const uint32_t DEFAULT_DAYLIGHT_OFFSET_SEC;

   private:
    bool timeSynced_;
    const char *ntpServer1_;
    const char *ntpServer2_;
    const char *ntpServer3_;
    uint32_t gmtOffsetSec_;
    uint32_t daylightOffsetSec_;
};