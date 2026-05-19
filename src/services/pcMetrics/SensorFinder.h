#pragma once

#include <ArduinoJson.h>
#include <vector>

/**
 * Utility class for finding sensors in Libre Hardware Monitor JSON structure.
 * Provides flexible matching for different hardware configurations.
 */
class SensorFinder {
 public:
    /**
     * Find a sensor by exact text match
     */
    static JsonObject findByExactName(JsonArray sensors, const char* name);

    /**
     * Find a sensor by checking if its Text field contains any of the patterns
     */
    static JsonObject findByPartialMatch(JsonArray sensors,
                                         const std::vector<const char*>& patterns);

    /**
     * Find a section (e.g., "Temperatures", "Load") within hardware children
     */
    static JsonObject findSection(JsonArray children, const char* sectionName);

    /**
     * Find first sensor whose Text field contains the given substring
     */
    static JsonObject findContaining(JsonArray sensors, const char* substring);

 private:
    static bool textContains(const char* text, const char* substring);
    static bool textMatches(const char* text, const std::vector<const char*>& patterns);

    // Core primitive: iterate `arr` and return the first element whose "Text"
    // field satisfies `pred(text)`.  All public find* methods delegate here.
    template <typename Predicate>
    static JsonObject findWhere(JsonArray arr, Predicate pred) {
        if (arr.isNull()) return JsonObject();
        for (JsonObject obj : arr) {
            const char* text = obj["Text"];
            if (text && pred(text)) return obj;
        }
        return JsonObject();
    }
};
