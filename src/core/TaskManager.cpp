#include "TaskManager.h"

#include <esp_task_wdt.h>

#include <climits>

#include "utils/LogMacros.h"

TaskManager::TaskManager(LoggerInterface& logger, IScreenUpdater& uiController,
                         const AppSettings& config, SystemState::ScreenState& screenState,
                         std::vector<BackgroundJob*> jobs)
    : logger_(logger),
      uiController_(uiController),
      config_(config),
      screenState_(screenState),
      jobs_(std::move(jobs)) {}

bool TaskManager::createTasks() {
    logger_.info("Initializing Application Tasks", true);

    bool success =
        createTask(updateScreenTask, SCREEN_TASK_NAME, config_.tasksScreenStack,
                   config_.tasksScreenPriority, &screenTaskHandle_, ARDUINO_RUNNING_CORE);

    if (!success) {
        logger_.critical("Failed to create screen update task", true);
        return false;
    }

    success = createTask(backgroundTask, BACKGROUND_TASK_NAME, config_.tasksBackgroundStack,
                         config_.tasksBackgroundPriority, &backgroundTaskHandle_, 0);

    if (!success) {
        logger_.critical("Failed to create background task", true);
        cleanup();
        return false;
    }

    if (config_.watchdogEnableOnBoot) {
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
    LOG_DEBUG(logger_, "Watchdog initialized for tasks", true);
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
    const TickType_t frequency = pdMS_TO_TICKS(config_.timingScreenTaskMs);
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
    const TickType_t frequency = pdMS_TO_TICKS(config_.timingBackgroundTaskMs);
    unsigned long lastStackLogTime = 0;

    while (true) {
        unsigned long now = millis();
        for (BackgroundJob* job : jobs_) {
            JobDue due = job->nextDue();
            bool isDue = false;
            switch (due.kind) {
                case JobDue::Kind::Never:
                    isDue = false;
                    break;
                case JobDue::Kind::Now:
                    isDue = true;
                    break;
                case JobDue::Kind::At:
                    // Wrap-safe: at 32-bit millis() rollover (~49.7 days),
                    // `now` can be smaller than `deadlineMs` while the
                    // deadline has still passed.
                    isDue = (long)(now - due.deadlineMs) >= 0;
                    break;
            }
            if (isDue) {
                job->run();
                // Feed the watchdog after every job, not just once per tick —
                // several blocking jobs (HTTP fetch, WiFi reconnect) coming
                // due on the same tick would otherwise stack up before a
                // single reset.
                resetWatchdog();
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
    LOG_DEBUGF(logger_, "%s stack high water mark: %u", taskName, stackHighWaterMark);
}

void TaskManager::resetWatchdog() {
    if (config_.watchdogEnableOnBoot) {
        esp_task_wdt_reset();
    }
}
