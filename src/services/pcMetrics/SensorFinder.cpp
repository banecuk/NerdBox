#include "SensorFinder.h"

JsonObject SensorFinder::findByExactName(JsonArray sensors, const char* name) {
    if (name == nullptr) return JsonObject();
    return findWhere(sensors, [name](const char* text) {
        return strcmp(text, name) == 0;
    });
}

JsonObject SensorFinder::findByPartialMatch(JsonArray sensors,
                                            const std::vector<const char*>& patterns) {
    if (patterns.empty()) return JsonObject();
    return findWhere(sensors, [&patterns](const char* text) {
        return textMatches(text, patterns);
    });
}

JsonObject SensorFinder::findSection(JsonArray children, const char* sectionName) {
    if (sectionName == nullptr) return JsonObject();
    return findWhere(children, [sectionName](const char* text) {
        return textContains(text, sectionName);
    });
}

JsonObject SensorFinder::findContaining(JsonArray sensors, const char* substring) {
    if (substring == nullptr) return JsonObject();
    return findWhere(sensors, [substring](const char* text) {
        return textContains(text, substring);
    });
}

bool SensorFinder::textContains(const char* text, const char* substring) {
    if (text == nullptr || substring == nullptr) return false;
    return strstr(text, substring) != nullptr;
}

bool SensorFinder::textMatches(const char* text, const std::vector<const char*>& patterns) {
    if (text == nullptr) return false;
    for (const char* pattern : patterns) {
        if (pattern && textContains(text, pattern)) return true;
    }
    return false;
}
