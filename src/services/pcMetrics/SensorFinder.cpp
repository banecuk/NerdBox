#include "SensorFinder.h"

JsonObject SensorFinder::findByExactName(JsonArray sensors, const char* name) {
    if (sensors.isNull() || name == nullptr) {
        return JsonObject();
    }

    for (JsonObject sensor : sensors) {
        const char* text = sensor["Text"];
        if (text && strcmp(text, name) == 0) {
            return sensor;
        }
    }
    return JsonObject();
}

JsonObject SensorFinder::findByPartialMatch(JsonArray sensors,
                                            const std::vector<const char*>& patterns) {
    if (sensors.isNull() || patterns.empty()) {
        return JsonObject();
    }

    for (JsonObject sensor : sensors) {
        const char* text = sensor["Text"];
        if (text && textMatches(text, patterns)) {
            return sensor;
        }
    }
    return JsonObject();
}

JsonObject SensorFinder::findSection(JsonArray children, const char* sectionName) {
    if (children.isNull() || sectionName == nullptr) {
        return JsonObject();
    }

    for (JsonObject child : children) {
        const char* text = child["Text"];
        if (text && textContains(text, sectionName)) {
            return child;
        }
    }
    return JsonObject();
}

JsonObject SensorFinder::findContaining(JsonArray sensors, const char* substring) {
    if (sensors.isNull() || substring == nullptr) {
        return JsonObject();
    }

    for (JsonObject sensor : sensors) {
        const char* text = sensor["Text"];
        if (text && textContains(text, substring)) {
            return sensor;
        }
    }
    return JsonObject();
}

bool SensorFinder::textContains(const char* text, const char* substring) {
    if (text == nullptr || substring == nullptr) {
        return false;
    }
    return strstr(text, substring) != nullptr;
}

bool SensorFinder::textMatches(const char* text, const std::vector<const char*>& patterns) {
    if (text == nullptr) {
        return false;
    }

    for (const char* pattern : patterns) {
        if (pattern && textContains(text, pattern)) {
            return true;
        }
    }
    return false;
}