#include "EventHandler.h"

#include <esp_system.h>

#include "ui/core/DisplayManager.h"
#include "ui/core/UiController.h"

EventHandler::EventHandler(UiController* uiController, LoggerInterface& logger)
    : uiController_(uiController), logger_(logger) {
    registerHandlers();
}

void EventHandler::registerHandlers() {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe(EventType::NONE,
                       [this]() { logger_.info("EventHandler- EventType::NONE called"); });

    eventBus.subscribe(EventType::RESET_DEVICE, [this]() { resetDevice(); });

    eventBus.subscribe(EventType::CYCLE_BRIGHTNESS, [this]() { cycleBrightness(); });

    eventBus.subscribe(EventType::SHOW_SETTINGS, [this]() { requestSettingsScreen(); });

    eventBus.subscribe(EventType::SHOW_MAIN, [this]() { requestMainScreen(); });
}

void EventHandler::resetDevice() {
    logger_.debug("RESET action received");

    if (uiController_->getDisplayManager()->getDisplay()) {
        LGFX* display = uiController_->getDisplayManager()->getDisplay();
        display->setTextColor(TFT_WHITE, TFT_BLACK);
        display->setTextSize(3);
        display->setTextDatum(TL_DATUM);
        display->drawString("RESETING DEVICE", 0, 0);
    }

    ESP.restart();
}

void EventHandler::cycleBrightness() {
    logger_.debug("BRIGHTNESS action received");
    uiController_->getDisplayManager()->cycleBrightness();
}

void EventHandler::requestSettingsScreen() {
    logger_.debug("SETTINGS action received");
    uiController_->requestScreen(ScreenName::SETTINGS);
}

void EventHandler::requestMainScreen() {
    logger_.debug("MAIN action received");
    uiController_->requestScreen(ScreenName::MAIN);
    // logger_.debugf("[Heap] Post-transition: %d", ESP.getFreeHeap());
}