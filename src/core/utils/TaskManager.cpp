#include "TaskManager.h"

TaskManager::TaskManager(ILogger &logger, ScreenManager &screenManager,
                         PcMetricsService &hmDataService, PcMetrics &hmData,
                         SystemState::CoreState &coreState,
                         SystemState::ScreenState &screenState)
    : logger_(logger),
      screenManager_(screenManager),
      hmDataService_(hmDataService),
      hmData_(hmData),
      coreState_(coreState),
      screenState_(screenState) {}

bool TaskManager::createTasks() {
    logger_.info("Initializing System Tasks", true);

    static constexpr UBaseType_t kScreenPriority = 3;
    static constexpr UBaseType_t kTouchPriority = 2;
    static constexpr UBaseType_t kBackgroundPriority = 1;

    BaseType_t screenTaskStatus = xTaskCreatePinnedToCore(
        updateScreenTask, "ScreenUpdate", Config::Tasks::kScreenStack, this,
        kScreenPriority, &screenTaskHandle, ARDUINO_RUNNING_CORE);

    if (screenTaskStatus != pdPASS) {
        logger_.critical("Failed to create screen update task! Error code: %d",
                         screenTaskStatus);
        return false;
    }

    BaseType_t touchTaskStatus = xTaskCreatePinnedToCore(
        touchTask, "TouchUpdate", Config::Tasks::kTouchStack, this, kTouchPriority,
        &touchTaskHandle, ARDUINO_RUNNING_CORE);

    if (screenTaskStatus != pdPASS) {
        logger_.critical("Failed to create touch task! Error code: %d", screenTaskStatus);
        return false;
    }

    BaseType_t backgroundTaskStatus = xTaskCreatePinnedToCore(
        backgroundTask, "BackgroundTask", Config::Tasks::kBackgroundStack, this,
        kBackgroundPriority, &backgroundTaskHandle, ARDUINO_RUNNING_CORE);

    if (backgroundTaskStatus != pdPASS) {
        logger_.critical("Failed to create background task! Error code: %d",
                         backgroundTaskStatus);
        if (screenTaskHandle != nullptr) {
            vTaskDelete(screenTaskHandle);
            screenTaskHandle = nullptr;
        }
        return false;
    }

    if (Config::Watchdog::kEnableOnBoot) {
        esp_task_wdt_add(screenTaskHandle);
        esp_task_wdt_add(backgroundTaskHandle);
    }

    return true;
}

void TaskManager::updateScreenTask(void *parameter) {
    auto *taskManager = static_cast<TaskManager *>(parameter);
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true) {
        if (taskManager->screenState_.isInitialized) {
            taskManager->screenManager_.draw();
            taskManager->resetWatchdog();
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::Timing::kScreenTaskMs));
    }
}

void TaskManager::touchTask(void *parameter) {
    auto *taskManager = static_cast<TaskManager *>(parameter);
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true) {
        if (taskManager->screenState_.isInitialized) {
            taskManager->screenManager_.handleTouchInput();
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(Config::Timing::kScreenTaskMs));
    }
}

void TaskManager::backgroundTask(void *parameter) {
    auto *taskManager = static_cast<TaskManager *>(parameter);
    while (true) {
        if (taskManager->coreState_.isInitialized && WiFi.status() == WL_CONNECTED) {
            if (taskManager->screenState_.activeScreen == ScreenName::MAIN) {
                if (millis() >= taskManager->coreState_.nextSync_HardwareMonitor) {
                    taskManager->updateHmData();
                    taskManager->resetWatchdog();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(Config::Timing::kBackgroundTaskMs));
    }
}

void TaskManager::resetWatchdog() {
    if (Config::Watchdog::kEnableOnBoot) {
        esp_task_wdt_reset();
    }
}

void TaskManager::updateHmData() {
    bool fetchSuccess = hmDataService_.fetchData(hmData_);
    if (fetchSuccess) {
        consecutiveFailures_ = 0;
        coreState_.nextSync_HardwareMonitor =
            millis() + Config::HardwareMonitor::kRefreshMs;
        logger_.debug("HM updated", true);
    } else {
        consecutiveFailures_++;
        coreState_.nextSync_HardwareMonitor =
            millis() + Config::HardwareMonitor::kRefreshAfterFailureMs;
        handleDownloadFailure();
        logger_.debug("HM update failed", true);
    }
}

void TaskManager::handleDownloadFailure() {
    if (consecutiveFailures_ >= Config::HardwareMonitor::kMaxRetries) {
        logger_.warning("Multiple consecutive HM failures detected");
        consecutiveFailures_ = 0;
        // screenManager.errorMessage("Data update failed");
    }
}