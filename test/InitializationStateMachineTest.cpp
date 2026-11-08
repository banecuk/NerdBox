#include "app/InitializationStateMachine.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// Records every log line as "LEVEL[*]: message" ('*' = forScreen) so tests
// can assert on the exact boot-screen messages and their order.
class RecordingLogger : public LoggerInterface {
 public:
    void debug(const char* m, bool s) override { add("DEBUG", m, s); }
    void info(const char* m, bool s) override { add("INFO", m, s); }
    void warning(const char* m, bool s) override { add("WARN", m, s); }
    void error(const char* m, bool s) override { add("ERROR", m, s); }
    void critical(const char* m, bool s) override { add("CRIT", m, s); }

#define REC_F(name, level)            \
    void name(const char* fmt, ...) override { \
        va_list args;                  \
        va_start(args, fmt);           \
        addf(level, fmt, args);        \
        va_end(args);                  \
    }
    REC_F(debugf, "DEBUG")
    REC_F(infof, "INFO")
    REC_F(warningf, "WARN")
    REC_F(errorf, "ERROR")
    REC_F(criticalf, "CRIT")
#undef REC_F

    // Lines excluding DEBUG — LOG_DEBUG call sites vanish when DEBUG_MODE=0.
    std::vector<std::string> nonDebug() const {
        std::vector<std::string> out;
        for (const auto& l : lines)
            if (l.rfind("DEBUG", 0) != 0)
                out.push_back(l);
        return out;
    }

    std::vector<std::string> lines;

 private:
    void add(const char* level, const char* m, bool s) {
        lines.push_back(std::string(level) + (s ? "*" : "") + ": " + m);
    }
    void addf(const char* level, const char* fmt, va_list args) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        add(level, buf, false);
    }
};

class FakeTarget : public IInitializationTarget {
 public:
    LoggerInterface& logger() override { return log; }

    void initializeDisplay() override { calls.push_back("initializeDisplay"); }
    void postInitializeDisplay() override { calls.push_back("postInitializeDisplay"); }
    bool initializeUi() override {
        calls.push_back("initializeUi");
        return uiOk;
    }
    void requestScreen(ScreenName screen) override {
        calls.push_back("requestScreen");
        requestedScreen = screen;
    }
    bool initWatchdog(uint32_t timeoutMs) override {
        calls.push_back("initWatchdog");
        watchdogTimeoutMs = timeoutMs;
        return watchdogOk;
    }
    bool addCurrentTaskToWatchdog() override {
        calls.push_back("addCurrentTaskToWatchdog");
        return watchdogOk;
    }
    bool createTasks() override {
        calls.push_back("createTasks");
        return tasksOk;
    }
    bool connectNetwork() override {
        calls.push_back("connectNetwork");
        return networkOk;
    }
    bool isNetworkConnected() const override { return networkOk; }
    bool syncTime() override {
        calls.push_back("syncTime");
        ++syncAttempts;
        return syncSucceedsOnAttempt != 0 && syncAttempts >= syncSucceedsOnAttempt;
    }
    void beginWebServer() override { calls.push_back("beginWebServer"); }
    void setScreenInitialized() override { calls.push_back("setScreenInitialized"); }
    void setSystemInitialized() override { calls.push_back("setSystemInitialized"); }

    void delayMs(uint32_t ms) override { delays.push_back(ms); }
    uint32_t randomBelow(uint32_t bound) override {
        jitterBounds.push_back(bound);
        return bound == 0 ? 0 : std::min(jitter, bound - 1);
    }
    uint32_t freeHeap() const override { return 123456; }

    RecordingLogger log;
    std::vector<std::string> calls;
    std::vector<uint32_t> delays;
    std::vector<uint32_t> jitterBounds;

    bool uiOk = true;
    bool watchdogOk = true;
    bool tasksOk = true;
    bool networkOk = true;
    int syncSucceedsOnAttempt = 1;  // 0 = never
    int syncAttempts = 0;
    uint32_t jitter = 7;
    uint32_t watchdogTimeoutMs = 0;
    ScreenName requestedScreen = ScreenName::NONE;
};

using State = InitializationStateMachine::State;

}  // namespace

TEST(InitializationStateMachineTest, HappyPathRunsStepsInOrder) {
    FakeTarget target;
    AppSettings config;
    InitializationStateMachine sm(target, config);

    EXPECT_EQ(sm.getCurrentState(), State::INITIAL);
    ASSERT_TRUE(sm.initialize());
    EXPECT_EQ(sm.getCurrentState(), State::COMPLETE);
    EXPECT_EQ(sm.progressPercent(), 100);

    const std::vector<std::string> expected = {
        "initializeDisplay", "initializeUi",          "initWatchdog",
        "createTasks",       "setScreenInitialized",  "connectNetwork",
        "syncTime",          "addCurrentTaskToWatchdog", "beginWebServer",
        "postInitializeDisplay", "requestScreen",     "setSystemInitialized",
    };
    EXPECT_EQ(target.calls, expected);
    EXPECT_EQ(target.requestedScreen, ScreenName::MAIN);
    EXPECT_EQ(target.watchdogTimeoutMs, config.watchdogTimeoutMs);
    EXPECT_TRUE(target.delays.empty());
}

TEST(InitializationStateMachineTest, HappyPathBootScreenMessages) {
    FakeTarget target;
    AppSettings config;
    InitializationStateMachine sm(target, config);
    ASSERT_TRUE(sm.initialize());

    const std::vector<std::string> expected = {
        "INFO*: Initialization state machine started",
        "INFO*: Initializing display",
        "INFO: Watchdog initialized with 20000ms timeout",
        "INFO*: Connecting to WiFi",
        "INFO*: Syncing time",
        "INFO*: Time synchronized successfully",
        "INFO*: HTTP Server started",
        "INFO*: Initialization state machine completed successfully",
    };
    EXPECT_EQ(target.log.nonDebug(), expected);
}

TEST(InitializationStateMachineTest, UiFailureIsFatalAndStopsSequence) {
    FakeTarget target;
    target.uiOk = false;
    AppSettings config;
    InitializationStateMachine sm(target, config);

    EXPECT_FALSE(sm.initialize());
    EXPECT_EQ(sm.getCurrentState(), State::FAILED);
    const std::vector<std::string> expectedCalls = {"initializeDisplay", "initializeUi"};
    EXPECT_EQ(target.calls, expectedCalls);

    const auto lines = target.log.nonDebug();
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(lines[lines.size() - 2], "CRIT*: UI initialization failed");
    EXPECT_EQ(lines.back(), "CRIT: Initialization failed in state: DISPLAY_INIT");
}

TEST(InitializationStateMachineTest, TaskCreationFailureIsFatal) {
    FakeTarget target;
    target.tasksOk = false;
    AppSettings config;
    InitializationStateMachine sm(target, config);

    EXPECT_FALSE(sm.initialize());
    EXPECT_EQ(sm.getCurrentState(), State::FAILED);
    EXPECT_EQ(target.calls.back(), "createTasks");  // no setScreenInitialized, no network
    EXPECT_EQ(target.log.nonDebug().back(), "CRIT: Initialization failed in state: TASKS_INIT");
}

TEST(InitializationStateMachineTest, InitializeIsIdempotentAfterTerminalState) {
    FakeTarget target;
    target.uiOk = false;
    AppSettings config;
    InitializationStateMachine sm(target, config);
    EXPECT_FALSE(sm.initialize());
    const size_t callsAfterFirst = target.calls.size();
    EXPECT_FALSE(sm.initialize());
    EXPECT_EQ(target.calls.size(), callsAfterFirst);
}

TEST(InitializationStateMachineTest, NetworkFailureIsNotFatal) {
    FakeTarget target;
    target.networkOk = false;
    AppSettings config;
    InitializationStateMachine sm(target, config);

    EXPECT_TRUE(sm.initialize());
    const auto lines = target.log.nonDebug();
    EXPECT_NE(std::find(lines.begin(), lines.end(), "WARN*: Network init failed, continuing"),
              lines.end());
    EXPECT_NE(std::find(lines.begin(), lines.end(), "WARN*: HTTP Server started (no network yet)"),
              lines.end());
}

// B6: a watchdog that can't be armed is reported once as critical, and boot
// carries on (not fatal).
TEST(InitializationStateMachineTest, WatchdogInitFailureIsReportedAndNotFatal) {
    FakeTarget target;
    target.watchdogOk = false;
    AppSettings config;
    InitializationStateMachine sm(target, config);

    EXPECT_TRUE(sm.initialize());
    const auto lines = target.log.nonDebug();
    int crit = 0;
    for (const auto& l : lines)
        if (l.rfind("CRIT", 0) == 0)
            ++crit;
    EXPECT_EQ(crit, 1);
    EXPECT_NE(
        std::find(lines.begin(), lines.end(), "CRIT*: Watchdog could not be armed, running without it"),
        lines.end());
    for (const auto& l : lines)
        EXPECT_EQ(l.find("Watchdog initialized"), std::string::npos) << l;
}

TEST(InitializationStateMachineTest, WatchdogDisabledSkipsAllWatchdogCalls) {
    FakeTarget target;
    AppSettings config;
    config.watchdogEnableOnBoot = false;
    InitializationStateMachine sm(target, config);

    EXPECT_TRUE(sm.initialize());
    for (const auto& c : target.calls) {
        EXPECT_NE(c, "initWatchdog");
        EXPECT_NE(c, "addCurrentTaskToWatchdog");
    }
    const auto lines = target.log.nonDebug();
    EXPECT_NE(std::find(lines.begin(), lines.end(), "INFO*: Watchdog disabled in configuration"),
              lines.end());
}

TEST(InitializationStateMachineTest, TimeSyncRetriesWithBackoffThenSucceeds) {
    FakeTarget target;
    target.syncSucceedsOnAttempt = 3;
    target.jitter = 7;
    AppSettings config;
    config.initTimeSyncRetries = 3;
    config.initTimeSyncBaseDelayMs = 100;
    config.initBackoffJitterMs = 50;
    InitializationStateMachine sm(target, config);

    EXPECT_TRUE(sm.initialize());
    EXPECT_EQ(target.syncAttempts, 3);
    const std::vector<uint32_t> expectedDelays = {107, 207};
    EXPECT_EQ(target.delays, expectedDelays);
    const std::vector<uint32_t> expectedBounds = {50, 50};
    EXPECT_EQ(target.jitterBounds, expectedBounds);
}

TEST(InitializationStateMachineTest, TimeSyncGivesUpWithoutTrailingDelay) {
    FakeTarget target;
    target.syncSucceedsOnAttempt = 0;
    target.jitter = 0;
    AppSettings config;
    config.initTimeSyncRetries = 3;
    config.initTimeSyncBaseDelayMs = 100;
    InitializationStateMachine sm(target, config);

    EXPECT_TRUE(sm.initialize());
    EXPECT_EQ(target.syncAttempts, 3);
    const std::vector<uint32_t> expectedDelays = {100, 200};  // none after the last attempt
    EXPECT_EQ(target.delays, expectedDelays);

    const auto lines = target.log.nonDebug();
    EXPECT_NE(std::find(lines.begin(), lines.end(), "WARN*: Time sync failed, using local time"),
              lines.end());
    // Main task still joins the watchdog after giving up.
    EXPECT_NE(std::find(target.calls.begin(), target.calls.end(), "addCurrentTaskToWatchdog"),
              target.calls.end());
}

// B62: a base delay wider than 16 bits must not be narrowed before the clamp.
TEST(InitializationStateMachineTest, BackoffKeepsBaseWiderThan16Bits) {
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(1, 20000, 0), 20000u);
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(1, 0x10010, 0),
              InitializationStateMachine::kMaxBackoffDelayMs);  // 65552 > 30000, not 16
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(1, 1000, 5), 1005u);
}

TEST(InitializationStateMachineTest, BackoffDoublesAndClamps) {
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(1, 100, 0), 100u);
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(2, 100, 0), 200u);
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(3, 100, 0), 400u);
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(0, 100, 0), 100u);
    // Huge attempt counts cap the shift and clamp rather than wrapping.
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(255, 100, 0),
              InitializationStateMachine::kMaxBackoffDelayMs);
    EXPECT_EQ(InitializationStateMachine::backoffDelayMs(255, 0xFFFFFFFFu, 0xFFFFFFFFu),
              InitializationStateMachine::kMaxBackoffDelayMs);
}

TEST(InitializationStateMachineTest, BackoffIsMonotonicAcrossAttempts) {
    uint32_t prev = 0;
    for (uint8_t attempt = 1; attempt < 40; ++attempt) {
        const uint32_t d = InitializationStateMachine::backoffDelayMs(attempt, 100, 0);
        EXPECT_GE(d, prev) << "attempt " << int(attempt);
        prev = d;
    }
}

TEST(InitializationStateMachineTest, StateNames) {
    EXPECT_STREQ(InitializationStateMachine::getStateName(State::INITIAL), "INITIAL");
    EXPECT_STREQ(InitializationStateMachine::getStateName(State::DISPLAY_INIT), "DISPLAY_INIT");
    EXPECT_STREQ(InitializationStateMachine::getStateName(State::FINAL_SETUP), "FINAL_SETUP");
    EXPECT_STREQ(InitializationStateMachine::getStateName(State::COMPLETE), "COMPLETE");
    EXPECT_STREQ(InitializationStateMachine::getStateName(State::FAILED), "FAILED");
}
