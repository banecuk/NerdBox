#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config/AppConfig.h"
#include "core/display/ScreenManager.h"
#include "core/system/SystemState.h"
#include "core/utils/Logger.h"
#include "services/PcMetrics.h"
#include "services/PcMetricsService.h"

class TaskManager {
   public:
    TaskManager(ILogger &logger, ScreenManager &screenManager,
                PcMetricsService &hmDataService, PcMetrics &hmData,
                SystemState::CoreState &system, SystemState::ScreenState &screenState);

    bool createTasks();
    static void updateScreenTask(void *parameter);
    static void touchTask(void *parameter);
    static void backgroundTask(void *parameter);

   private:
    TaskHandle_t screenTaskHandle;
    TaskHandle_t touchTaskHandle;
    TaskHandle_t backgroundTaskHandle;

    ILogger &logger_;
    ScreenManager &screenManager_;
    PcMetricsService &hmDataService_;
    PcMetrics &hmData_;
    uint8_t consecutiveFailures_ = 0;

    SystemState::CoreState &coreState_;
    SystemState::ScreenState &screenState_;

    void updateHmData();
    void handleDownloadFailure();
    void resetWatchdog();
};

#endif