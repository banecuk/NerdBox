#include "UiEventHandler.h"

#include <esp_system.h>

#include "core/resources/FontRegistry.h"
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
    logger_.debug("RESET action received");

    if (uiController_->getDisplayManager()->getDisplay()) {
        LGFX* display = uiController_->getDisplayManager()->getDisplay();
        Fonts::loadMetric(display);
        display->setTextColor(TFT_WHITE, TFT_BLACK);
        display->setTextDatum(TL_DATUM);
        display->drawString("RESETING DEVICE", 0, 0);
        Fonts::unload(display);
    }

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
