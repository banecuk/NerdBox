#include "Application.h"

#include "ApplicationComponents.h"

Application::Application(std::unique_ptr<ApplicationComponents> components)
    : components_(std::move(components)),
      config_(components_->config),
      systemState_(components_->systemState),
      webServerService_(components_->webServerService) {}

bool Application::initialize() {
    return components_->initStateMachine.initialize();
}

void Application::run() {
    if (!systemState_.core.isInitialized) {
        return;
    }

    if (config_.watchdogEnableOnBoot) {
        esp_task_wdt_reset();
    }

    webServerService_.processRequests();
    vTaskDelay(pdMS_TO_TICKS(config_.timingMainLoopMs));
}