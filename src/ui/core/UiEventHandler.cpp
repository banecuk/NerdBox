#include "UiEventHandler.h"

#include <esp_system.h>

#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"
#include "utils/LogMacros.h"

UiEventHandler::UiEventHandler(UiController* uiController, LoggerInterface& logger)
    : uiController_(uiController), logger_(logger) {
    registerHandlers();
}

UiEventHandler::~UiEventHandler() {
    auto& eventBus = EventBus::getInstance();
    for (const auto& sub : subscriptions_) {
        eventBus.unsubscribe(sub.type, sub.id);
    }
}

void UiEventHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    size_t i = 0;
    subscriptions_[i++] = {
        EventType::NONE, eventBus.subscribe(EventType::NONE, [this]() {
            logger_.info("UiEventHandler: EventType::NONE received");
        })};
    subscriptions_[i++] = {EventType::RESET_DEVICE,
                           eventBus.subscribe(EventType::RESET_DEVICE,
                                              [this]() { resetDevice(); })};
    subscriptions_[i++] = {EventType::CYCLE_BRIGHTNESS,
                           eventBus.subscribe(EventType::CYCLE_BRIGHTNESS,
                                              [this]() { cycleBrightness(); })};
    subscriptions_[i++] = {EventType::SHOW_SETTINGS,
                           eventBus.subscribe(EventType::SHOW_SETTINGS,
                                              [this]() { requestSettingsScreen(); })};
    subscriptions_[i++] = {EventType::SHOW_MAIN,
                           eventBus.subscribe(EventType::SHOW_MAIN,
                                              [this]() { requestMainScreen(); })};
    subscriptions_[i++] = {EventType::SHOW_GAME,
                           eventBus.subscribe(EventType::SHOW_GAME,
                                              [this]() { requestGameScreen(); })};
    subscriptions_[i++] = {EventType::SHOW_DISKS,
                           eventBus.subscribe(EventType::SHOW_DISKS,
                                              [this]() { requestDisksScreen(); })};
    subscriptions_[i++] = {EventType::SHOW_WEATHER,
                           eventBus.subscribe(EventType::SHOW_WEATHER,
                                              [this]() { requestWeatherScreen(); })};
}

void UiEventHandler::resetDevice() {
    // No on-screen message: drawing here would race the screen task (no
    // lock), and with no delay before restart() it would never actually be
    // visible anyway.
    LOG_DEBUG(logger_, "RESET action received");
    ESP.restart();
}

void UiEventHandler::cycleBrightness() {
    LOG_DEBUG(logger_, "BRIGHTNESS action received");
    uiController_->getDisplayManager()->cycleBrightness();
}

void UiEventHandler::requestSettingsScreen() {
    LOG_DEBUG(logger_, "SETTINGS action received");
    uiController_->requestScreen(ScreenName::SETTINGS);
}

void UiEventHandler::requestMainScreen() {
    LOG_DEBUG(logger_, "MAIN action received");
    uiController_->requestScreen(ScreenName::MAIN);
}

void UiEventHandler::requestGameScreen() {
    LOG_DEBUG(logger_, "GAME action received");
    uiController_->requestScreen(ScreenName::GAME);
}

void UiEventHandler::requestDisksScreen() {
    LOG_DEBUG(logger_, "DISKS action received");
    uiController_->requestScreen(ScreenName::DISKS);
}

void UiEventHandler::requestWeatherScreen() {
    LOG_DEBUG(logger_, "WEATHER action received");
    uiController_->requestScreen(ScreenName::WEATHER);
}
