#include "EventBus.h"

// Singleton instance
EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

// Constructor
EventBus::EventBus() = default;

// Destructor
EventBus::~EventBus() = default;

// Subscribe to an event
void EventBus::subscribe(const std::string& eventName, EventCallback callback) {
    subscribers[eventName].push_back(callback);
}

// Publish an event
void EventBus::publish(const std::string& eventName) {
    if (subscribers.find(eventName) != subscribers.end()) {
        for (auto& callback : subscribers[eventName]) {
            callback();
        }
    }
}