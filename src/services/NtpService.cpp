#include "NtpService.h"

#include <WiFi.h>

const char* NtpService::DEFAULT_NTP_SERVER1 = "europe.pool.ntp.org";
const char* NtpService::DEFAULT_NTP_SERVER2 = "time.google.com";
const char* NtpService::DEFAULT_NTP_SERVER3 = "time.cloudflare.com";

NtpService::NtpService()
    : timeSynced_(false),
      ntpServer1_(DEFAULT_NTP_SERVER1),
      ntpServer2_(DEFAULT_NTP_SERVER2),
      ntpServer3_(DEFAULT_NTP_SERVER3) {}

bool NtpService::syncTime(const char* timezone) {
    // configTzTime sets the POSIX TZ rule and starts SNTP in one call, instead
    // of the old configTime(UTC-offset) + setenv("TZ") + tzset() combo, which
    // applied a fixed GMT/DST offset and then immediately overrode it.
    configTzTime(timezone, ntpServer1_, ntpServer2_, ntpServer3_);

    // Wait for time to synchronize. Each attempt is bounded by
    // kSyncAttemptTimeoutMs (not the getLocalTime() default of 5000ms), so the
    // worst case here is maxRetries * kSyncAttemptTimeoutMs rather than
    // several minutes.
    struct tm timeinfo;
    int retries = 0;
    const int maxRetries = 20;
    while (!getLocalTime(&timeinfo, kSyncAttemptTimeoutMs)) {
        if (retries++ >= maxRetries) {
            timeSynced_ = false;
            return false;
        }
    }

    timeSynced_ = true;
    return true;
}

bool NtpService::retrySyncIfNeeded() {
    if (timeSynced_) {
        return true;
    }

    unsigned long now = millis();
    if (now - lastRetryAttemptMs_ < kRetryIntervalMs) {
        return false;
    }
    lastRetryAttemptMs_ = now;

    // SNTP was already started by the initial syncTime() call and keeps
    // retrying in the background at its own pace, so this just probes
    // whether it has resolved the time yet — no need to call configTzTime()
    // again, and no need to block waiting for it: a 0ms non-blocking read
    // (same as getTime()'s) is enough, since kRetryIntervalMs's own cadence
    // is what does the "retrying" (see B22 — a 200ms blocking wait here
    // stalled the shared background task, including SseConnection::poll(),
    // every 30s while unsynced).
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, kNonBlockingReadTimeoutMs)) {
        timeSynced_ = true;
        return true;
    }
    return false;
}

bool NtpService::isTimeSynced() const {
    return timeSynced_;
}

struct tm NtpService::getTime() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, kNonBlockingReadTimeoutMs)) {
        memset(&timeinfo, 0, sizeof(timeinfo));
    }
    return timeinfo;
}
