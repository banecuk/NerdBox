#pragma once

// Explicit result of BackgroundJob::nextDue() — replaces the old single
// unsigned long return that conflated "run now" (0), "never / precondition
// unmet" (ULONG_MAX), and "absolute deadline" in one integer. That encoding
// is what produced the WeatherJob backoff bug (returning 0 for the midnight
// rollover case bypassed its own failure-backoff deadline) — see
// docs-local/02-architecture.md A2. Use the JobDue::now()/never()/at()
// factories rather than constructing JobDue directly.
struct JobDue {
    enum class Kind { Never, Now, At };

    Kind kind;
    unsigned long deadlineMs = 0;  // only meaningful when kind == At

    static JobDue never() { return {Kind::Never}; }
    static JobDue now() { return {Kind::Now}; }
    static JobDue at(unsigned long deadlineMs) { return {Kind::At, deadlineMs}; }
};

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

    // When run() should next be called. Re-evaluated every tick, so a job
    // can also use this to gate on non-time preconditions (e.g. JobDue::never()
    // while WiFi is down) rather than just a delay.
    virtual JobDue nextDue() const = 0;

    virtual void run() = 0;
};
