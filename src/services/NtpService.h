#pragma once

#include <atomic>
#include <cstdint>

#include "config/AppSettings.h"

#include <time.h>

class NtpService {
 public:
    // timeSynced is SystemState::core.isTimeSynced: the one "time synced" flag
    // in the system. NtpService is its only writer.
    NtpService(std::atomic<bool>& timeSynced, const AppSettings& config);
    bool syncTime(const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3");

    // Cheap, rate-limited retry for the case where the initial boot-time sync
    // (InitializationStateMachine::handleTimeInit) exhausted its retries.
    // Safe to call every background-task tick; only actually probes SNTP
    // every config.ntpRetryIntervalMs; each probe is a 0 ms non-blocking read.
    bool retrySyncIfNeeded();

    bool isTimeSynced() const;
    struct tm getTime() const;

    static const char* DEFAULT_NTP_SERVER1;
    static const char* DEFAULT_NTP_SERVER2;
    static const char* DEFAULT_NTP_SERVER3;

 private:
    // Not a tuning knob: getTime() and the background probe must never
    // block, so this has to be 0. (The boot-time per-attempt wait and the
    // background retry spacing are knobs: config_.ntpSyncAttemptTimeoutMs /
    // ntpRetryIntervalMs.)
    static constexpr uint32_t kNonBlockingReadTimeoutMs = 0;

    std::atomic<bool>& timeSynced_;
    const AppSettings& config_;
    const char* ntpServer1_;
    const char* ntpServer2_;
    const char* ntpServer3_;
    unsigned long lastRetryAttemptMs_ = 0;
};