#include "UiEventHandler.h"

#include <esp_system.h>

#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"

UiEventHandler::UiEventHandler(UiController* uiController, LoggerInterface& logger)
    : uiController_(uiController), logger_(logger) {
    registerHandlers();
}

void UiEventHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(EventType::NONE,
                       [this]() { logger_.info("UiEventHandler: EventType::NONE received"); });

    eventBus.subscribe(EventType::RESET_DEVICE,    [this]() { resetDevice(); });
    eventBus.subscribe(EventType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); });
    eventBus.subscribe(EventType::SHOW_SETTINGS,   [this]() { requestSettingsScreen(); });
    eventBus.subscribe(EventType::SHOW_MAIN,       [this]() { requestMainScreen(); });
}

void UiEventHandler::resetDevice() {
    // No on-screen message: drawing here would race the screen task (no
    // lock), and with no delay before restart() it would never actually be
    // visible anyway.
    logger_.debug("RESET action received");
    ESP.restart();
}

void UiEventHandler::cycleBrightness() {
    logger_.debug("BRIGHTNESS action received");
    uiController_->getDisplayManager()->cycleBrightness();
}

void UiEventHandler::requestSettingsScreen() {
    logger_.debug("SETTINGS action received");
    uiController_->requestScreen(ScreenName::SETTINGS);
}

void UiEventHandler::requestMainScreen() {
    logger_.debug("MAIN action received");
    uiController_->requestScreen(ScreenName::MAIN);
}
