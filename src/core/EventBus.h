#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <functional>
#include <map>
#include <vector>
#include "ActionTypes.h"

class EventBus {
public:
    // Singleton instance
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    // Event handler type
    using EventCallback = std::function<void()>;

    // Subscribe to an event
    void subscribe(ActionType actionType, EventCallback callback) {
        subscribers[actionType].push_back(callback);
    }

    // Publish an event
    void publish(ActionType actionType) {
        if (subscribers.find(actionType) != subscribers.end()) {
            for (auto& callback : subscribers[actionType]) {
                callback();
            }
        }
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    std::map<ActionType, std::vector<EventCallback>> subscribers;
};

#endif // EVENT_BUS_H