#pragma once

// Implemented by anything that needs periodic execution on TaskManager's
// background task. TaskManager holds a flat list of BackgroundJob* and
// iterates it every tick instead of hard-coding each service's scheduling
// and gating logic — adding a new periodic service means writing one job
// adapter and registering it in ApplicationComponents, not touching
// TaskManager's constructor or executeBackgroundTask().
//
// Blocking budget: TaskManager feeds the watchdog once per due job (see
// TaskManager::executeBackgroundTask()), not once per tick, so several jobs
// coming due on the same tick can't starve the watchdog between them — but
// a single run() call is still on the clock. Keep any one run() call well
// under WatchdogImpl::kTimeoutMs (20 s default); a job that can block that
// long on its own (e.g. a stalled socket) should apply its own internal
// timeout rather than relying on the watchdog to catch it.
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
