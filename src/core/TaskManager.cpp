#include "TaskManager.h"

#include <esp_task_wdt.h>

TaskManager::TaskManager(LoggerInterface& logger, UiController& uiController,
                         PcMetricsService& pcMetricsService, PcMetrics& pcMetrics,
                         SystemState::CoreState& coreState,
                         SystemState::ScreenState& screenState,
                         AppConfigInterface& config)
    : logger_(logger),
      uiController_(uiController),
      pcMetricsService_(pcMetricsService),
      pcMetrics_(pcMetrics),
      coreState_(coreState),
      screenState_(screenState),
      config_(config) {}

bool TaskManager::createTasks() {
    logger_.info("Initializing Application Tasks", true);

    bool success = createTask(
        updateScreenTask, SCREEN_TASK_NAME, config_.getTasksScreenStack(),
        config_.getTasksScreenPriority(), &screenTaskHandle_, ARDUINO_RUNNING_CORE);

    if (!success) {
        logger_.critical("Failed to create screen update task", true);
        return false;
    }

    success = createTask(backgroundTask, BACKGROUND_TASK_NAME,
                         config_.getTasksBackgroundStack(),
                         config_.getTasksBackgroundPriority(), &backgroundTaskHandle_,
                         0);  // Core 0

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

bool TaskManager::createTask(TaskFunction_t taskFunction, const char* taskName,
                             uint32_t stackSize, UBaseType_t priority,
                             TaskHandle_t* taskHandle, BaseType_t coreId) {
    BaseType_t status = xTaskCreatePinnedToCore(taskFunction, taskName, stackSize, this,
                                                priority, taskHandle, coreId);

    if (status != pdPASS) {
        // Use the formatted version for including error code
        logger_.criticalf("Failed to create task: %s, error: %d", taskName, status);
        return false;
    }

    logger_.info("Task created successfully: " + String(taskName), true);
    return true;
}

void TaskManager::initializeWatchdog() {
    if (screenTaskHandle_ != nullptr) {
        esp_task_wdt_add(screenTaskHandle_);
    }
    if (backgroundTaskHandle_ != nullptr) {
        esp_task_wdt_add(backgroundTaskHandle_);
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
            resetWatchdog();
        }

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
        if (coreState_.isInitialized && WiFi.status() == WL_CONNECTED) {
            if (screenState_.activeScreen == ScreenName::MAIN) {
                if (millis() >= coreState_.nextSync_pcMetrics) {
                    updatePcMetrics();
                    resetWatchdog();
                }
            }
        }

        // Periodic stack monitoring
        if (millis() - lastStackLogTime >= STACK_MONITOR_INTERVAL_MS) {
            logStackHighWaterMark(BACKGROUND_TASK_NAME);
            lastStackLogTime = millis();
        }

        vTaskDelay(frequency);
    }
}

void TaskManager::logStackHighWaterMark(const char* taskName) {
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
    // Use the formatted version for including the number
    logger_.debugf("%s stack high water mark: %u", taskName, stackHighWaterMark);
}

void TaskManager::updatePcMetrics() {
    bool fetchSuccess = pcMetricsService_.fetchData(pcMetrics_);

    if (fetchSuccess) {
        consecutiveFailures_ = 0;
        coreState_.nextSync_pcMetrics = millis() + config_.getHardwareMonitorRefreshMs();
        logger_.debug("PC metrics updated successfully", true);
    } else {
        consecutiveFailures_++;
        coreState_.nextSync_pcMetrics =
            millis() + config_.getHardwareMonitorRefreshAfterFailureMs();
        handlePcMetricsFailure();
    }
}

void TaskManager::handlePcMetricsFailure() {
    logger_.debug("PC metrics update failed", true);

    if (consecutiveFailures_ >= config_.getHardwareMonitorMaxRetries()) {
        logger_.warning("Multiple consecutive PC metrics failures detected", true);
        consecutiveFailures_ = 0;
    }
}

void TaskManager::resetWatchdog() {
    if (config_.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }
}