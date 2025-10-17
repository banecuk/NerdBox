#include "Application.h"

#include "ApplicationComponents.h"

Application::Application(std::unique_ptr<ApplicationComponents> components)
    : components_(std::move(components)) {}

bool Application::initialize() {
    return components_->initStateMachine.initialize();
}

void Application::run() {
    if (!components_->systemState.core.isInitialized) {
        return;
    }

    if (components_->config.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }

    components_->webServerService.processRequests();
    vTaskDelay(components_->config.getTimingMainLoopMs());
}