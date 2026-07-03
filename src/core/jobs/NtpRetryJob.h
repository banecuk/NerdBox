#pragma once

#include <climits>

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "services/NtpService.h"
#include "utils/LoggerInterface.h"

// Retries NTP sync in the background if the boot-time attempt
// (InitializationStateMachine) exhausted its retries. NtpService
// internally rate-limits actual sync probes.
class NtpRetryJob : public BackgroundJob {
 public:
    NtpRetryJob(NtpService& ntpService, SystemState::CoreState& coreState, LoggerInterface& logger)
        : ntpService_(ntpService), coreState_(coreState), logger_(logger) {}

    unsigned long nextDueMs() const override {
        return (coreState_.isInitialized && !coreState_.isTimeSynced) ? 0 : ULONG_MAX;
    }

    void run() override {
        if (ntpService_.retrySyncIfNeeded()) {
            coreState_.isTimeSynced = true;
            logger_.info("Time synchronized (background retry)", true);
        }
    }

 private:
    NtpService& ntpService_;
    SystemState::CoreState& coreState_;
    LoggerInterface& logger_;
};
