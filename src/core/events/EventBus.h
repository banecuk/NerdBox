#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "EventTypes.h"

// Subscribe only during init, from objects that live for the app's lifetime.
// A short-lived subscriber must call unsubscribe() with its token before it
// is destroyed, or its captured `this` dangles the next time the event fires.
class EventBus {
 public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    using EventCallback = std::function<void()>;
    using SubscriptionId = uint32_t;

    // Returns a token that can be passed to unsubscribe() to remove this
    // callback again.
    SubscriptionId subscribe(EventType type, EventCallback callback) {
        MutexGuard guard(mutex_);
        const SubscriptionId id = nextId_++;
        subscribers_[index(type)].push_back({id, std::move(callback)});
        return id;
    }

    void unsubscribe(EventType type, SubscriptionId id) {
        MutexGuard guard(mutex_);
        auto& subs = subscribers_[index(type)];
        subs.erase(std::remove_if(subs.begin(), subs.end(),
                                  [id](const Subscription& s) { return s.id == id; }),
                   subs.end());
    }

    // Copies the callback list under the lock, then invokes callbacks
    // outside it — publish() and subscribe()/unsubscribe() can safely race
    // from different tasks without callbacks running while the mutex (and
    // any lock a callback itself takes) is held.
    void publish(EventType type) {
        std::vector<EventCallback> callbacks;
        {
            MutexGuard guard(mutex_);
            const auto& subs = subscribers_[index(type)];
            callbacks.reserve(subs.size());
            for (const auto& sub : subs) {
                callbacks.push_back(sub.callback);
            }
        }

        for (auto& callback : callbacks) {
            callback();
        }
    }

 private:
    static constexpr size_t EVENT_TYPE_COUNT = static_cast<size_t>(EventType::COUNT);

    static constexpr size_t index(EventType type) { return static_cast<size_t>(type); }

    struct Subscription {
        SubscriptionId id;
        EventCallback callback;
    };

    struct MutexGuard {
        explicit MutexGuard(SemaphoreHandle_t mutex) : mutex_(mutex) {
            xSemaphoreTake(mutex_, portMAX_DELAY);
        }
        ~MutexGuard() { xSemaphoreGive(mutex_); }
        MutexGuard(const MutexGuard&) = delete;
        MutexGuard& operator=(const MutexGuard&) = delete;
        SemaphoreHandle_t mutex_;
    };

    std::array<std::vector<Subscription>, EVENT_TYPE_COUNT> subscribers_;
    SemaphoreHandle_t mutex_ = xSemaphoreCreateMutex();
    SubscriptionId nextId_ = 1;

    EventBus() = default;
    ~EventBus() { vSemaphoreDelete(mutex_); }
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
};