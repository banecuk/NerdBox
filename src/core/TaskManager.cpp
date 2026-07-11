#include "TaskManager.h"

#include <esp_task_wdt.h>

#include <climits>

TaskManager::TaskManager(LoggerInterface& logger, IScreenUpdater& uiController,
                         AppConfigInterface& config, SystemState::ScreenState& screenState,
                         std::vector<BackgroundJob*> jobs)
    : logger_(logger),
      uiController_(uiController),
      config_(config),
      screenState_(screenState),
      jobs_(std::move(jobs)) {}

bool TaskManager::createTasks() {
    logger_.info("Initializing Application Tasks", true);

    bool success =
        createTask(updateScreenTask, SCREEN_TASK_NAME, config_.getTasksScreenStack(),
                   config_.getTasksScreenPriority(), &screenTaskHandle_, ARDUINO_RUNNING_CORE);

    if (!success) {
        logger_.critical("Failed to create screen update task", true);
        return false;
    }

    success = createTask(backgroundTask, BACKGROUND_TASK_NAME, config_.getTasksBackgroundStack(),
                         config_.getTasksBackgroundPriority(), &backgroundTaskHandle_, 0);

    if (!success) {
        logger_.critical("Failed to create background task", true);
        cleanup();
        return false;
    }

    if (config_.getWatchdogEnableOnBoot()) {
        initializeWatchdog();
    }

    logger_.info("All tasks created successfully", true);
    return true;
}

void TaskManager::cleanup() {
    if (screenTaskHandle_ != nullptr) {
        vTaskDelete(screenTaskHandle_);
        screenTaskHandle_ = nullptr;
    }

    if (backgroundTaskHandle_ != nullptr) {
        vTaskDelete(backgroundTaskHandle_);
        backgroundTaskHandle_ = nullptr;
    }
}

bool TaskManager::createTask(TaskFunction_t taskFunction, const char* taskName, uint32_t stackSize,
                             UBaseType_t priority, TaskHandle_t* taskHandle, BaseType_t coreId) {
    BaseType_t status = xTaskCreatePinnedToCore(taskFunction, taskName, stackSize, this, priority,
                                                taskHandle, coreId);

    if (status != pdPASS) {
        logger_.criticalf("Failed to create task: %s, error: %d", taskName, status);
        return false;
    }

    logger_.infof("Task created successfully: %s", taskName);
    return true;
}

void TaskManager::initializeWatchdog() {
    if (screenTaskHandle_ != nullptr) {
        esp_err_t ret = esp_task_wdt_add(screenTaskHandle_);
        if (ret != ESP_OK) {
            logger_.errorf("Failed to add %s to watchdog: %s", SCREEN_TASK_NAME,
                           esp_err_to_name(ret));
        }
    }
    if (backgroundTaskHandle_ != nullptr) {
        esp_err_t ret = esp_task_wdt_add(backgroundTaskHandle_);
        if (ret != ESP_OK) {
            logger_.errorf("Failed to add %s to watchdog: %s", BACKGROUND_TASK_NAME,
                           esp_err_to_name(ret));
        }
    }
    logger_.debug("Watchdog initialized for tasks", true);
}

void TaskManager::updateScreenTask(void* parameter) {
    auto* taskManager = static_cast<TaskManager*>(parameter);
    taskManager->executeScreenTask();
}

void TaskManager::backgroundTask(void* parameter) {
    auto* taskManager = static_cast<TaskManager*>(parameter);
    taskManager->executeBackgroundTask();
}

void TaskManager::executeScreenTask() {
    const TickType_t frequency = pdMS_TO_TICKS(config_.getTimingScreenTaskMs());
    TickType_t lastWakeTime = xTaskGetTickCount();
    unsigned long lastStackLogTime = 0;

    while (true) {
        if (screenState_.isInitialized) {
            uiController_.updateDisplay();
        }

        // Feed the watchdog every tick regardless of init state.
        resetWatchdog();

        // Periodic stack monitoring
        if (millis() - lastStackLogTime >= STACK_MONITOR_INTERVAL_MS) {
            logStackHighWaterMark(SCREEN_TASK_NAME);
            lastStackLogTime = millis();
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void TaskManager::executeBackgroundTask() {
    const TickType_t frequency = pdMS_TO_TICKS(config_.getTimingBackgroundTaskMs());
    unsigned long lastStackLogTime = 0;

    while (true) {
        unsigned long now = millis();
        for (BackgroundJob* job : jobs_) {
            unsigned long dueAt = job->nextDueMs();
            // Wrap-safe: at 32-bit millis() rollover (~49.7 days), `now` can
            // be smaller than `dueAt` while the deadline has still passed.
            // ULONG_MAX is the "not eligible" sentinel (see BackgroundJob.h)
            // and must be excluded — it would otherwise wrap to a due
            // deadline as soon as `now` is small.
            if (dueAt != ULONG_MAX && (long)(now - dueAt) >= 0) {
                job->run();
            }
        }

        resetWatchdog();

        if (millis() - lastStackLogTime >= STACK_MONITOR_INTERVAL_MS) {
            logStackHighWaterMark(BACKGROUND_TASK_NAME);
            lastStackLogTime = millis();
        }

        vTaskDelay(frequency);
    }
}

void TaskManager::logStackHighWaterMark(const char* taskName) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
    logger_.debugf("%s stack high water mark: %u", taskName, stackHighWaterMark);
}

void TaskManager::resetWatchdog() {
    if (config_.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }
}
