#pragma once

// Wrap-safe "don't retry until N ms after the last failure" timer.
//
// Background jobs used to store an absolute `millis() + backoff` deadline,
// written only on failure and starting at 0, and hand it to
// JobScheduler::isDue() as JobDue::at(deadline). isDue()'s signed-difference
// compare is only correct while the deadline is within ±2^31 ms (~24.8 days)
// of now, so a deadline that is never refreshed (no failure for 24.8 days)
// flips to "in the future" and the job stops running for the next 24.8 days
// (B35).
//
// Cooldown instead stores when the backoff *started* and compares elapsed
// time (`now - startMs < durationMs`, unsigned), which is correct across the
// rollover. deadlineMs() is only meaningful while active(), and then it is
// always within durationMs of now — safe to pass to JobDue::at().
//
// Pure, no Arduino dependency — callers pass `now`. Host-tested
// (test/CooldownTest.cpp).
class Cooldown {
 public:
    void start(unsigned long nowMs, unsigned long durationMs) {
        startMs_ = nowMs;
        durationMs_ = durationMs;
        pending_ = true;
    }

    void clear() { pending_ = false; }

    bool active(unsigned long nowMs) const {
        return pending_ && (nowMs - startMs_) < durationMs_;
    }

    unsigned long deadlineMs() const { return startMs_ + durationMs_; }

 private:
    unsigned long startMs_ = 0;
    unsigned long durationMs_ = 0;
    bool pending_ = false;
};
