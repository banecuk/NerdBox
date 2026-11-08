#pragma once

#include "core/BackgroundJob.h"
#include "core/state/SystemState.h"
#include "services/NtpService.h"
#include "utils/logging/LoggerInterface.h"

// Retries NTP sync in the background if the boot-time attempt
// (InitializationStateMachine) exhausted its retries. NtpService
// internally rate-limits actual sync probes.
class NtpRetryJob : public BackgroundJob {
 public:
    NtpRetryJob(NtpService& ntpService, SystemState::CoreState& coreState, LoggerInterface& logger)
        : ntpService_(ntpService), coreState_(coreState), logger_(logger) {}

    JobDue nextDue() const override {
        return (coreState_.isInitialized && !coreState_.isTimeSynced) ? JobDue::now()
                                                                      : JobDue::never();
    }

    void run() override {
        // retrySyncIfNeeded() sets coreState_.isTimeSynced itself (NtpService
        // is that flag's only writer), which also stops nextDue() firing.
        if (ntpService_.retrySyncIfNeeded()) {
            logger_.info("Time synchronized (background retry)", true);
        }
    }

 private:
    NtpService& ntpService_;
    SystemState::CoreState& coreState_;
    LoggerInterface& logger_;
};
