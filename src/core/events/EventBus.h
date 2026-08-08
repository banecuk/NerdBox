#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "EventTypes.h"

// Subscribe only during init, from objects that live for the app's lifetime.
// A short-lived subscriber must call unsubscribe() with its token before it
// is destroyed, or its captured `this` dangles the next time the event fires.
//
// Caveat: unsubscribe() only guarantees the callback won't be *copied* into
// a future publish() after it returns — it does not wait for a publish()
// already in flight on another core. That publish() took its own copy of
// the callback list before releasing the mutex (see publish()'s comment) and
// keeps invoking those copies outside the lock, so a callback can still run
// after its unsubscribe() call has completed. Safe today because every
// subscriber is app-lifetime and never actually unsubscribes before its own
// destruction happens after all tasks stop; a subscriber that unsubscribes
// while still alive and expects "no more calls after this returns" would
// need publish() to invoke under the lock instead.
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
    // any lock a callback itself takes) is held. See the class-level comment
    // for what this copy-then-invoke design means for unsubscribe() timing.
    //
    // Each call heap-allocates a std::vector<EventCallback> copy of the
    // subscriber list. Fine for rare UI-driven events (button taps, screen
    // transitions); do not call this from a hot path (per-frame/per-tick
    // code) — use a direct call or a polled flag there instead.
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