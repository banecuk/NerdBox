#include <climits>

#include "core/JobScheduler.h"

#include <gtest/gtest.h>

namespace {

class FakeJob : public BackgroundJob {
 public:
    explicit FakeJob(JobDue due) : due_(due) {}

    JobDue nextDue() const override { return due_; }
    void run() override { runCount_++; }

    JobDue due_;
    int runCount_ = 0;
};

}  // namespace

TEST(JobSchedulerTest, NeverIsNotDue) {
    EXPECT_FALSE(JobScheduler::isDue(JobDue::never(), 12345));
}

TEST(JobSchedulerTest, NowIsAlwaysDue) {
    EXPECT_TRUE(JobScheduler::isDue(JobDue::now(), 0));
    EXPECT_TRUE(JobScheduler::isDue(JobDue::now(), ULONG_MAX));
}

TEST(JobSchedulerTest, AtIsDueOnceDeadlineReached) {
    JobDue due = JobDue::at(1000);
    EXPECT_FALSE(JobScheduler::isDue(due, 999));
    EXPECT_TRUE(JobScheduler::isDue(due, 1000));
    EXPECT_TRUE(JobScheduler::isDue(due, 1001));
}

TEST(JobSchedulerTest, AtSurvivesMillisRollover) {
    // now has wrapped past ULONG_MAX back to a small value, while the
    // deadline was set just before the wrap — the deadline has still
    // passed and this must be treated as due, not skipped for another
    // ~49.7 days.
    unsigned long deadline = ULONG_MAX - 10;
    unsigned long now = 5;
    EXPECT_TRUE(JobScheduler::isDue(JobDue::at(deadline), now));
}

TEST(JobSchedulerTest, AtNotYetDueNearRolloverBoundary) {
    unsigned long deadline = 5;
    unsigned long now = ULONG_MAX - 10;
    EXPECT_FALSE(JobScheduler::isDue(JobDue::at(deadline), now));
}

TEST(JobSchedulerTest, TickRunsOnlyDueJobs) {
    FakeJob dueNow(JobDue::now());
    FakeJob notDue(JobDue::never());
    FakeJob dueAtPast(JobDue::at(100));
    std::vector<BackgroundJob*> jobs = {&dueNow, &notDue, &dueAtPast};

    JobScheduler scheduler;
    scheduler.tick(jobs, 200);

    EXPECT_EQ(dueNow.runCount_, 1);
    EXPECT_EQ(notDue.runCount_, 0);
    EXPECT_EQ(dueAtPast.runCount_, 1);
}

TEST(JobSchedulerTest, TickInvokesOnJobRunOncePerRunJob) {
    FakeJob dueNow(JobDue::now());
    FakeJob notDue(JobDue::never());
    std::vector<BackgroundJob*> jobs = {&dueNow, &notDue};

    JobScheduler scheduler;
    int callbackCount = 0;
    scheduler.tick(jobs, 0, [&callbackCount] { callbackCount++; });

    EXPECT_EQ(callbackCount, 1);
}
