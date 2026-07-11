#pragma once

#include <time.h>

class NtpService {
 public:
    NtpService();
    bool syncTime(const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3");

    // Cheap, rate-limited retry for the case where the initial boot-time sync
    // (InitializationStateMachine::handleTimeInit) exhausted its retries.
    // Safe to call every background-task tick; only actually probes SNTP
    // every kRetryIntervalMs, and each probe is bounded by kSyncAttemptTimeoutMs.
    bool retrySyncIfNeeded();

    bool isTimeSynced() const;
    struct tm getTime() const;

    static const char* DEFAULT_NTP_SERVER1;
    static const char* DEFAULT_NTP_SERVER2;
    static const char* DEFAULT_NTP_SERVER3;

 private:
    // Non-blocking reads (getTime()) must never stall the caller.
    static constexpr uint32_t kNonBlockingReadTimeoutMs = 0;
    // Each attempt in the boot-time retry loop and the periodic background
    // retry — short because SNTP has either already resolved the time or not;
    // waiting longer per attempt just blocks the caller for no benefit.
    static constexpr uint32_t kSyncAttemptTimeoutMs = 200;
    // Minimum spacing between periodic re-sync probes so retrySyncIfNeeded()
    // can be called every background-task tick without hammering SNTP.
    static constexpr unsigned long kRetryIntervalMs = 30000;

    bool timeSynced_;
    const char* ntpServer1_;
    const char* ntpServer2_;
    const char* ntpServer3_;
    unsigned long lastRetryAttemptMs_ = 0;
};