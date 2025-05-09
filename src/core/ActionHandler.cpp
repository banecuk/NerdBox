#include "ActionHandler.h"

#include <ESP.h>

#include "ui/DisplayDriver.h"
#include "ui/UIController.h"

ActionHandler::ActionHandler(UIController* uiController, ILogger& logger)
    : uiController_(uiController), logger_(logger) {
    registerHandlers();
}

void ActionHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(ActionType::NONE, [this]() {
        logger_.info("ActionHandler- ActionType::NONE called");
    });

    eventBus.subscribe(ActionType::RESET_DEVICE, [this]() { resetDevice(); });

    eventBus.subscribe(ActionType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); });

    eventBus.subscribe(ActionType::SHOW_SETTINGS, [this]() { requestSettingsScreen(); });

    eventBus.subscribe(ActionType::SHOW_MAIN, [this]() { requestMainScreen(); });
}

void ActionHandler::resetDevice() {
    logger_.debug("RESET action received");

    if (uiController_->getDisplayDriver()->getDisplay()) {
        LGFX* display = uiController_->getDisplayDriver()->getDisplay();
        display->setTextColor(TFT_WHITE, TFT_BLACK);
        display->setTextSize(3);
        display->setTextDatum(TL_DATUM);
        display->drawString("RESETING DEVICE", 0, 0);
    }

    ESP.restart();
}

void ActionHandler::cycleBrightness() {
    logger_.debug("BRIGHTNESS action received");
    uiController_->getDisplayDriver()->cycleBrightness();
}

void ActionHandler::requestSettingsScreen() {
    logger_.debug("SETTINGS action received");
    uiController_->requestScreen(ScreenName::SETTINGS);
}

void ActionHandler::requestMainScreen() {
    logger_.debug("MAIN action received");
    uiController_->requestScreen(ScreenName::MAIN);
    // logger_.debugf("[Heap] Post-transition: %d", ESP.getFreeHeap());
}