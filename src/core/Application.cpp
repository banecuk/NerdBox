#include "Application.h"

#include "ApplicationComponents.h"

// Updated constructor - now extremely simple!
Application::Application(std::unique_ptr<ApplicationComponents> components)
    : components_(std::move(components)) {}

bool Application::initialize() {
    // Delegate all initialization to the state machine
    return components_->initStateMachine.initialize();
}

void Application::run() {
    if (!components_->systemState.core.isInitialized) {
        return;
    }

    if (components_->config.getWatchdogEnableOnBoot()) {
        esp_task_wdt_reset();
    }

    components_->httpServer.processRequests();
    vTaskDelay(components_->config.getTimingMainLoopMs());
}