#include "TaskManager.h"

#include <esp_task_wdt.h>

TaskManager::TaskManager(LoggerInterface& logger, UIController& uiController,
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

    BaseType_t screenTaskStatus = xTaskCreatePinnedToCore(
        updateScreenTask, "ScreenUpdate", config_.getTasksScreenStack(), this,
        config_.getTasksScreenPriority(), &screenTaskHandle, ARDUINO_RUNNING_CORE);

    if (screenTaskStatus != pdPASS) {
        logger_.critical("Failed to create screen update task! Error code: %d",
                         screenTaskStatus);
        return false;
    }

    BaseType_t backgroundTaskStatus = xTaskCreatePinnedToCore(
        backgroundTask, "BackgroundTask", config_.getTasksBackgroundStack(), this,
        config_.getTasksBackgroundPriority(), &backgroundTaskHandle, 0);
    if (backgroundTaskStatus != pdPASS) {
        logger_.critical("Failed to create background task! Error code: %d",
                         backgroundTaskStatus);
        if (screenTaskHandle != nullptr) {
            vTaskDelete(screenTaskHandle);
            screenTaskHandle = nullptr;
        }
        return false;
    }

    if (config_.getWatchdogEnableOnBoot()) {
        esp_task_wdt_add(screenTaskHandle);
        esp_task_wdt_add(backgroundTaskHandle);
    }

    return true;
}

void TaskManager::updateScreenTask(void* parameter) {
    auto* taskManager = static_cast<TaskManager*>(parameter);
    TickType_t lastWakeTime = xTaskGetTickCount();

    const TickType_t frequency =
        pdMS_TO_TICKS(taskManager->config_.getTimingScreenTaskMs());
    unsigned long lastLogTime = 0;
    const unsigned long logIntervalMs = 20000;  // Log every 20 seconds

    while (true) {
        if (taskManager->screenState_.isInitialized) {
            taskManager->uiController_.updateDisplay();
            taskManager->resetWatchdog();

            // Log stack high water mark periodically
            unsigned long currentTime = millis();
            if (currentTime - lastLogTime >= logIntervalMs) {
                UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
                taskManager->logger_.debugf("Screen task stack high water mark: %u",
                                            stackHighWaterMark);
                lastLogTime = currentTime;
            }
        }
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void TaskManager::backgroundTask(void* parameter) {
    auto* taskManager = static_cast<TaskManager*>(parameter);
    const TickType_t frequency =
        pdMS_TO_TICKS(taskManager->config_.getTimingBackgroundTaskMs());
    unsigned long lastLogTime = 0;
    const unsigned long logIntervalMs = 20000;  // Log every 20 seconds

    while (true) {
        if (taskManager->coreState_.isInitialized && WiFi.status() == WL_CONNECTED) {
            if (taskManager->screenState_.activeScreen == ScreenName::MAIN) {
                if (millis() >= taskManager->coreState_.nextSync_pcMetrics) {
                    taskManager->updatePcMetrics();
                    taskManager->resetWatchdog();
                }
            }
        }

        // Log stack high water mark periodically
        unsigned long currentTime = millis();
        if (currentTime - lastLogTime >= logIntervalMs) {
            UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(nullptr);
            taskManager->logger_.debugf("Background task stack high water mark: %u",
                                        stackHighWaterMark);
            lastLogTime = currentTime;
        }

        vTaskDelay(frequency);
    }
}

void TaskManager::resetWatchdog() {
    if (config_.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }
}

void TaskManager::updatePcMetrics() {
    bool fetchSuccess = pcMetricsService_.fetchData(pcMetrics_);
    if (fetchSuccess) {
        consecutiveFailures_ = 0;
        coreState_.nextSync_pcMetrics = millis() + config_.getHardwareMonitorRefreshMs();
    } else {
        consecutiveFailures_++;
        coreState_.nextSync_pcMetrics =
            millis() + config_.getHardwareMonitorRefreshAfterFailureMs();
        handleDownloadFailure();
        logger_.debug("HM update failed", true);
    }
}

void TaskManager::handleDownloadFailure() {
    if (consecutiveFailures_ >= config_.getHardwareMonitorMaxRetries()) {
        logger_.warning("Multiple consecutive HM failures detected");
        consecutiveFailures_ = 0;
    }
}