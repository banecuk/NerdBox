#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <functional>
#include <map>
#include <vector>
#include <string>

class EventBus {
public:
    // Singleton instance
    static EventBus& getInstance();

    // Event handler type
    using EventCallback = std::function<void()>;

    // Subscribe to an event
    void subscribe(const std::string& eventName, EventCallback callback);

    // Publish an event
    void publish(const std::string& eventName);

private:
    EventBus();
    ~EventBus();
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    std::map<std::string, std::vector<EventCallback>> subscribers;
};

#endif // EVENT_BUS_H