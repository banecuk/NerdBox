#pragma once

#include <Arduino.h>

#include <vector>

#include "config/AppSettings.h"
#include "core/BackgroundJob.h"
#include "core/IScreenUpdater.h"
#include "core/state/SystemState.h"
#include "utils/Logger.h"

class TaskManager {
 public:
    // `jobs` is a flat list of periodic background jobs (see BackgroundJob) —
    // adding a new periodic service means writing a job adapter and appending
    // it here, not adding a constructor parameter.
    TaskManager(LoggerInterface& logger, IScreenUpdater& uiController, const AppSettings& config,
                SystemState::ScreenState& screenState, std::vector<BackgroundJob*> jobs);

    bool createTasks();
    void cleanup();

    // Task entry points (keep these public and static for FreeRTOS)
    static void updateScreenTask(void* parameter);
    static void backgroundTask(void* parameter);

 private:
    // Constants
    static constexpr const char* SCREEN_TASK_NAME = "ScreenUpdate";
    static constexpr const char* BACKGROUND_TASK_NAME = "BackgroundTask";
    static constexpr unsigned long STACK_MONITOR_INTERVAL_MS = 20000;

    // Dependencies
    LoggerInterface& logger_;
    IScreenUpdater& uiController_;
    const AppSettings& config_;
    SystemState::ScreenState& screenState_;
    std::vector<BackgroundJob*> jobs_;

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

    void initializeWatchdog();
    void logStackHighWaterMark(const char* taskName);
    void resetWatchdog();

    // Non-copyable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
};
