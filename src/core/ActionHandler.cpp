#include "ActionHandler.h"

#include <ESP.h>

ActionHandler::ActionHandler(ScreenManager* screenManager, ILogger& logger,
                             DisplayManager* displayManager)
    : screenManager(screenManager), logger_(logger), displayManager_(displayManager) {
    registerHandlers();
}

void ActionHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(ActionType::NONE, [this]() {
        logger_.info("ActionHandler- ActionType::NONE called");
    });

    eventBus.subscribe(ActionType::RESET_DEVICE, [this]() { resetDevice(); });

    eventBus.subscribe(ActionType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); });

    // eventBus.subscribe(ActionType::SHOW_SETTINGS, [this]() {
    //     showSettings();
    // });

    // eventBus.subscribe(ActionType::SHOW_ABOUT, [this]() {
    //     showAbout();
    // });
}

void ActionHandler::resetDevice() { ESP.restart(); }

void ActionHandler::cycleBrightness() {
    displayManager_->cycleBrightness();
}