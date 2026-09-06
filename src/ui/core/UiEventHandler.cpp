#include "UiEventHandler.h"

#include <esp_system.h>

#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"
#include "utils/logging/LogMacros.h"

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
    for (const auto& descriptor : kScreens) {
        if (descriptor.event == EventType::NONE) continue;  // BOOT has no event
        const ScreenName screen = descriptor.screen;
        const EventType event = descriptor.event;
        subscriptions_[i++] = {event, eventBus.subscribe(event, [this, screen]() {
                                    LOG_DEBUGF(logger_, "SHOW_%s action received",
                                               screenName(screen));
                                    uiController_->requestScreen(screen);
                                })};
    }

    subscriptions_[i++] = {EventType::NONE, eventBus.subscribe(EventType::NONE, [this]() {
                               logger_.info("UiEventHandler: EventType::NONE received");
                           })};
    subscriptions_[i++] = {
        EventType::RESET_DEVICE,
        eventBus.subscribe(EventType::RESET_DEVICE, [this]() { resetDevice(); })};
    subscriptions_[i++] = {
        EventType::CYCLE_BRIGHTNESS,
        eventBus.subscribe(EventType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); })};
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
