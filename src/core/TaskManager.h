#pragma once

#include <Arduino.h>

#include "config/AppConfigInterface.h"
#include "core/state/SystemState.h"
#include "services/pcMetrics/PcMetrics.h"
#include "services/pcMetrics/PcMetricsService.h"
#include "ui/UiController.h"
#include "utils/Logger.h"

class TaskManager {
 public:
    TaskManager(LoggerInterface& logger, UiController& uiController,
                PcMetricsService& pcMetricsService, PcMetrics& pcMetrics,
                SystemState::CoreState& coreState, SystemState::ScreenState& screenState,
                AppConfigInterface& config);

    bool createTasks();  // Public method name matches your existing code
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
    UiController& uiController_;
    PcMetricsService& pcMetricsService_;
    PcMetrics& pcMetrics_;
    SystemState::CoreState& coreState_;
    SystemState::ScreenState& screenState_;
    AppConfigInterface& config_;

    // Task management
    TaskHandle_t screenTaskHandle_ = nullptr;
    TaskHandle_t backgroundTaskHandle_ = nullptr;
    uint8_t consecutiveFailures_ = 0;

    // Task implementations
    void executeScreenTask();
    void executeBackgroundTask();

    // Helper methods
    bool createTask(TaskFunction_t taskFunction, const char* taskName, uint32_t stackSize,
                    UBaseType_t priority, TaskHandle_t* taskHandle,
                    BaseType_t coreId = tskNO_AFFINITY);

    void initializeWatchdog();
    void logStackHighWaterMark(const char* taskName);
    void updatePcMetrics();
    void handlePcMetricsFailure();
    void resetWatchdog();

    // Non-copyable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
};