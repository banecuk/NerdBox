#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config/AppConfig.h"
#include "core/state/SystemState.h"
#include "services/PcMetrics.h"
#include "services/PcMetricsService.h"
#include "ui/UIController.h"
#include "utils/Logger.h"

class TaskManager {
   public:
    TaskManager(LoggerInterface &logger, UIController &uiController,
                PcMetricsService &pcMetricsService, PcMetrics &pcMetrics,
                SystemState::CoreState &coreState, SystemState::ScreenState &screenState);

    bool createTasks();
    static void updateScreenTask(void *parameter);
    static void backgroundTask(void *parameter);

   private:
    TaskHandle_t screenTaskHandle = nullptr;
    TaskHandle_t backgroundTaskHandle = nullptr;

    LoggerInterface &logger_;
    UIController &uiController_;
    PcMetricsService &pcMetricsService_;
    PcMetrics &pcMetrics_;
    uint8_t consecutiveFailures_ = 0;

    SystemState::CoreState &coreState_;
    SystemState::ScreenState &screenState_;

    void updatePcMetrics();
    void handleDownloadFailure();
    void resetWatchdog();
};