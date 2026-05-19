#pragma once

#include <array>
#include <functional>
#include <vector>

#include "EventTypes.h"

class EventBus {
 public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    using EventCallback = std::function<void()>;

    void subscribe(EventType type, EventCallback callback) {
        subscribers_[index(type)].push_back(std::move(callback));
    }

    void publish(EventType type) {
        for (auto& callback : subscribers_[index(type)]) {
            callback();
        }
    }

 private:
    static constexpr size_t EVENT_TYPE_COUNT = static_cast<size_t>(EventType::COUNT);

    static constexpr size_t index(EventType type) { return static_cast<size_t>(type); }

    std::array<std::vector<EventCallback>, EVENT_TYPE_COUNT> subscribers_;

    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
};