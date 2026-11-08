#pragma once

#include <Arduino.h>

#include <atomic>
#include <vector>

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/IScreenUpdater.h"
#include "core/ITaskStackReporter.h"
#include "core/JobScheduler.h"
#include "core/state/SystemState.h"
#include "utils/logging/Logger.h"

class TaskManager : public ITaskStackReporter {
 public:
    // `jobs` is a flat list of periodic background jobs (see BackgroundJob) —
    // adding a new periodic service means writing a job adapter and appending
    // it here, not adding a constructor parameter.
    TaskManager(LoggerInterface& logger, IScreenUpdater& uiController, const AppSettings& config,
                SystemState::ScreenState& screenState, std::vector<BackgroundJob*> jobs);

    bool createTasks();
    void cleanup();

    // --- Watchdog (B6) ------------------------------------------------------
    // TaskManager owns the task watchdog and tracks what is *actually* armed,
    // rather than trusting AppSettings::watchdogEnableOnBoot (a compile-time
    // constant that stays true even when arming failed). Each task only
    // resets the WDT once it was successfully added to it, so a failed init
    // or add can't turn into a stream of esp_task_wdt_reset() errors.
    //
    // Arms the WDT (esp_task_wdt_init). Call before createTasks(). Returns
    // whether it's armed; on false, createTasks() adds nothing to it.
    bool initWatchdog(uint32_t timeoutMs);
    // Adds the calling task (setup/loop) to the WDT. No-op returning false if
    // initWatchdog() didn't succeed.
    bool addCurrentTaskToWatchdog();
    // Feeds the WDT for the calling (main loop) task, only if it joined.
    void feedMainTaskWatchdog();
    // True once the WDT is armed and at least one task is subscribed to it.
    bool watchdogActive() const override {
        return screenTaskWatched_ || backgroundTaskWatched_ || mainTaskWatched_;
    }

    // Task entry points (keep these public and static for FreeRTOS)
    static void updateScreenTask(void* parameter);
    static void backgroundTask(void* parameter);

    // Exposed for /api/status — nullptr before createTasks() succeeds.
    TaskHandle_t getScreenTaskHandle() const { return screenTaskHandle_; }
    TaskHandle_t getBackgroundTaskHandle() const { return backgroundTaskHandle_; }

    // ITaskStackReporter — 0 means the task hasn't been created yet.
    uint32_t screenTaskStackFree() const override {
        return screenTaskHandle_ ? uxTaskGetStackHighWaterMark(screenTaskHandle_) : 0;
    }
    uint32_t backgroundTaskStackFree() const override {
        return backgroundTaskHandle_ ? uxTaskGetStackHighWaterMark(backgroundTaskHandle_) : 0;
    }

 private:
    // Constants
    static constexpr const char* kScreenTaskName = "ScreenUpdate";
    static constexpr const char* kBackgroundTaskName = "BackgroundTask";
    static constexpr unsigned long kStackMonitorIntervalMs = 20000;

    // Dependencies
    LoggerInterface& logger_;
    IScreenUpdater& uiController_;
    const AppSettings& config_;
    SystemState::ScreenState& screenState_;
    std::vector<BackgroundJob*> jobs_;
    JobScheduler jobScheduler_;

    // Task management
    TaskHandle_t screenTaskHandle_ = nullptr;
    TaskHandle_t backgroundTaskHandle_ = nullptr;

    // Task implementations
    void executeScreenTask();
    void executeBackgroundTask();

    // Helper methods
    bool createTask(TaskFunction_t taskFunction, const char* taskName, uint32_t stackSize,
                    UBaseType_t priority, TaskHandle_t* taskHandle,
                    BaseType_t coreId = tskNO_AFFINITY);

    void addTasksToWatchdog();
    bool addTaskToWatchdog(TaskHandle_t handle, const char* taskName);
    void logStackHighWaterMark(const char* taskName);
    static void resetWatchdog(const std::atomic<bool>& taskWatched);

    // Watchdog runtime state — see initWatchdog(). Written on the setup task,
    // read on the screen/background/main-loop tasks.
    bool watchdogInitialized_ = false;
    std::atomic<bool> screenTaskWatched_{false};
    std::atomic<bool> backgroundTaskWatched_{false};
    std::atomic<bool> mainTaskWatched_{false};

    // Non-copyable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
};
