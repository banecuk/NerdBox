#pragma once

// Implemented by anything that needs periodic execution on TaskManager's
// background task. TaskManager holds a flat list of BackgroundJob* and
// iterates it every tick instead of hard-coding each service's scheduling
// and gating logic — adding a new periodic service means writing one job
// adapter and registering it in ApplicationComponents, not touching
// TaskManager's constructor or executeBackgroundTask().
class BackgroundJob {
 public:
    virtual ~BackgroundJob() = default;

    // Absolute millis() timestamp at which run() should next be called.
    // Re-evaluated every tick, so a job can also use this to gate on
    // non-time preconditions (e.g. return ULONG_MAX while WiFi is down)
    // rather than just a delay.
    virtual unsigned long nextDueMs() const = 0;

    virtual void run() = 0;
};
