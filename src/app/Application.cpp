#include "Application.h"

#include "ApplicationComponents.h"

Application::Application(std::unique_ptr<ApplicationComponents> components)
    : components_(std::move(components)),
      config_(components_->config),
      webServerService_(components_->webServerService),
      taskManager_(components_->taskManager) {}

bool Application::initialize() {
    return components_->initStateMachine.initialize();
}

void Application::run() {
    // Resets only if the main task really joined an armed watchdog (B6).
    taskManager_.feedMainTaskWatchdog();

    webServerService_.processRequests();
    vTaskDelay(pdMS_TO_TICKS(config_.timingMainLoopMs));
}