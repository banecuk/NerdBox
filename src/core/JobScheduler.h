#pragma once

#include <vector>

#include "core/BackgroundJob.h"

// Pure due-evaluation + dispatch loop, extracted from
// TaskManager::executeBackgroundTask() so the JobDue tri-state and the
// millis() rollover comparison are host-testable without FreeRTOS.
class JobScheduler {
 public:
    // Wrap-safe: at 32-bit millis() rollover (~49.7 days), `now` can be
    // smaller than `due.deadlineMs` while the deadline has still passed.
    static bool isDue(const JobDue& due, unsigned long now) {
        switch (due.kind) {
            case JobDue::Kind::Never:
                return false;
            case JobDue::Kind::Now:
                return true;
            case JobDue::Kind::At:
                return (long)(now - due.deadlineMs) >= 0;
        }
        return false;
    }

    // Runs every job whose nextDue() has arrived. onJobRun is invoked after
    // each run() call — TaskManager uses it to feed the watchdog per job
    // rather than once per tick, so several jobs coming due on the same tick
    // can't starve it between them.
    template <typename OnJobRun>
    void tick(const std::vector<BackgroundJob*>& jobs, unsigned long now, OnJobRun onJobRun) {
        for (BackgroundJob* job : jobs) {
            if (isDue(job->nextDue(), now)) {
                job->run();
                onJobRun();
            }
        }
    }

    void tick(const std::vector<BackgroundJob*>& jobs, unsigned long now) {
        tick(jobs, now, [] {});
    }
};
